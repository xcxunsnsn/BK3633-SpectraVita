import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:universal_ble/universal_ble.dart';

import '../protocol/spectravita_protocol.dart';

typedef BleCommandExecutor = Future<ProtocolFrame> Function(
  Uint8List command, {
  required int expectedResponse,
  required Duration timeout,
});

class BleController extends ChangeNotifier {
  BleController({bool initializeBle = true, this.commandExecutor}) {
    for (final sensor in SensorType.values) {
      records[sensor] = <ParsedRecord>[];
      packetCounts[sensor] = 0;
    }
    if (initializeBle) {
      _scanSubscription = UniversalBle.scanStream.listen(_onScanResult);
      _availabilitySubscription = UniversalBle.availabilityStream.listen((
        value,
      ) {
        availability = value;
        notifyListeners();
      });
    }
  }

  static const serviceShortUuid = 'fee0';
  static const writeShortUuid = 'fee2';
  static const notifyShortUuid = 'fee4';

  final Map<String, BleDevice> _devicesById = {};
  final Map<SensorType, List<ParsedRecord>> records = {};
  final Map<SensorType, int> packetCounts = {};
  final Map<SensorType, SingleMeasurement> singleResults = {};
  final Set<SensorType> activeSchedules = <SensorType>{};
  final List<String> logLines = [];
  final BleCommandExecutor? commandExecutor;

  StreamSubscription<BleDevice>? _scanSubscription;
  StreamSubscription<AvailabilityState>? _availabilitySubscription;
  StreamSubscription<bool>? _connectionSubscription;
  StreamSubscription<Uint8List>? _valueSubscription;
  Timer? _scanTimer;
  Timer? _uploadQuietTimer;
  Completer<ProtocolFrame>? _responseCompleter;
  int? _expectedResponse;

  AvailabilityState availability = AvailabilityState.unknown;
  BleDevice? connectedDevice;
  bool scanning = false;
  bool connecting = false;
  bool scheduleCommandActive = false;
  bool collectionStateUnknown = false;
  final Set<SensorType> staleSingleResults = <SensorType>{};
  SensorType? uploadingSensor;
  SensorType? measuringSensor;
  SensorType selectedSensor = SensorType.spectrum;
  int spectrumGainCode = DeviceCommands.spectrumGain;
  int flickerGainCode = DeviceCommands.flickerGain;
  int ultravioletGainCode = DeviceCommands.ultravioletGain;
  int proximityGainCode = 3;
  String status = '等待连接设备';
  String? lastError;
  int? negotiatedMtu;
  int _uploadPacketCount = 0;
  bool _commandExecutorConnected = true;

  String? _serviceUuid;
  String? _writeUuid;

  List<BleDevice> get devices {
    final result = _devicesById.values.toList();
    result.sort((a, b) => (b.rssi ?? -999).compareTo(a.rssi ?? -999));
    return result;
  }

  bool get connected =>
      (commandExecutor != null && _commandExecutorConnected) ||
      (connectedDevice != null && _writeUuid != null);
  bool get running => activeSchedules.isNotEmpty;
  bool get busy =>
      connecting ||
      uploadingSensor != null ||
      measuringSensor != null ||
      scheduleCommandActive;
  int get uploadPacketCount => _uploadPacketCount;

  void selectSensor(SensorType sensor) {
    selectedSensor = sensor;
    notifyListeners();
  }

  void setProximityGainCode(int value) {
    if (value < 0 || value > 3 || proximityGainCode == value) return;
    proximityGainCode = value;
    notifyListeners();
  }

  void setSpectrumGainCode(int value) {
    if (value < 0 ||
        value >= spectrumGainMultipliers.length ||
        spectrumGainCode == value) {
      return;
    }
    spectrumGainCode = value;
    notifyListeners();
  }

  void setFlickerGainCode(int value) {
    if (value < 0 ||
        value >= spectrumGainMultipliers.length ||
        flickerGainCode == value) {
      return;
    }
    flickerGainCode = value;
    notifyListeners();
  }

  void setUltravioletGainCode(int value) {
    if (value < 0 ||
        value >= ultravioletGainMultipliers.length ||
        ultravioletGainCode == value) {
      return;
    }
    ultravioletGainCode = value;
    notifyListeners();
  }

  Future<void> startScan() async {
    if (scanning) return;
    lastError = null;
    _devicesById.clear();
    scanning = true;
    status = '正在扫描 BLE 设备…';
    notifyListeners();
    try {
      await UniversalBle.requestPermissions();
      await UniversalBle.startScan();
      _scanTimer?.cancel();
      _scanTimer = Timer(const Duration(seconds: 12), stopScan);
    } catch (error) {
      scanning = false;
      _reportError('扫描失败', error);
      rethrow;
    }
  }

  Future<void> stopScan() async {
    _scanTimer?.cancel();
    if (!scanning) return;
    try {
      await UniversalBle.stopScan();
    } finally {
      scanning = false;
      status = connected
          ? '已连接 ${connectedDevice!.name ?? connectedDevice!.deviceId}'
          : '扫描结束';
      notifyListeners();
    }
  }

  void _onScanResult(BleDevice device) {
    _devicesById[device.deviceId] = device;
    notifyListeners();
  }

  Future<void> connect(BleDevice device) async {
    if (connecting) return;
    connecting = true;
    lastError = null;
    status = '正在连接 ${device.name ?? device.deviceId}…';
    notifyListeners();
    try {
      await stopScan();
      if (connectedDevice != null) await disconnect();

      await _connectionSubscription?.cancel();
      _connectionSubscription = UniversalBle.connectionStream(device.deviceId)
          .listen((isConnected) {
            if (!isConnected && connectedDevice?.deviceId == device.deviceId) {
              _handleDisconnected('设备连接已断开');
            }
          });
      await UniversalBle.connect(
        device.deviceId,
        timeout: const Duration(seconds: 20),
      );

      final services = await UniversalBle.discoverServices(device.deviceId);
      final service = services
          .where((item) => _matchesShortUuid(item.uuid, serviceShortUuid))
          .firstOrNull;
      if (service == null) {
        throw const ProtocolException('设备不包含 FEE0 服务');
      }
      final write = service.characteristics
          .where((item) => _matchesShortUuid(item.uuid, writeShortUuid))
          .firstOrNull;
      final notify = service.characteristics
          .where((item) => _matchesShortUuid(item.uuid, notifyShortUuid))
          .firstOrNull;
      if (write == null || notify == null) {
        throw const ProtocolException('设备缺少 FEE2 写入或 FEE4 通知特征');
      }

      _serviceUuid = service.uuid;
      _writeUuid = write.uuid;
      connectedDevice = device;

      await _valueSubscription?.cancel();
      _valueSubscription = UniversalBle.characteristicValueStream(
        device.deviceId,
        notify.uuid,
      ).listen(_onNotification);
      await UniversalBle.subscribeNotifications(
        device.deviceId,
        service.uuid,
        notify.uuid,
      );

      try {
        negotiatedMtu = await UniversalBle.requestMtu(device.deviceId, 517);
      } catch (error) {
        negotiatedMtu = null;
        _addLog('MTU 由系统管理：$error');
      }
      status = collectionStateUnknown
          ? '已重新连接；设备周期采集可能仍在继续，请先停止确认'
          : '已连接 ${device.name ?? device.deviceId}';
      _addLog('已连接，FEE2 写入 / FEE4 通知，MTU ${negotiatedMtu ?? '未知'}');
    } catch (error) {
      _handleDisconnected('连接失败');
      _reportError('连接失败', error);
      rethrow;
    } finally {
      connecting = false;
      notifyListeners();
    }
  }

  Future<void> disconnect() async {
    final device = connectedDevice;
    if (device == null) {
      if (commandExecutor != null && _commandExecutorConnected) {
        _commandExecutorConnected = false;
        _handleDisconnected('已断开连接');
      }
      return;
    }
    try {
      await UniversalBle.disconnect(device.deviceId);
    } finally {
      _handleDisconnected('已断开连接');
    }
  }

  void _handleDisconnected(String message) {
    _uploadQuietTimer?.cancel();
    if (_responseCompleter case final completer? when !completer.isCompleted) {
      completer.completeError(StateError('BLE 已断开'));
    }
    _responseCompleter = null;
    _expectedResponse = null;
    collectionStateUnknown = collectionStateUnknown || running;
    staleSingleResults.addAll(singleResults.keys);
    connectedDevice = null;
    _serviceUuid = null;
    _writeUuid = null;
    negotiatedMtu = null;
    uploadingSensor = null;
    measuringSensor = null;
    scheduleCommandActive = false;
    status = collectionStateUnknown ? '$message；设备周期采集可能仍在继续' : message;
    notifyListeners();
  }

  Future<SingleMeasurement> captureOnce([SensorType? requestedSensor]) async {
    _requireConnected();
    if (busy) throw StateError('当前有任务正在执行');
    if (running || collectionStateUnknown) {
      throw StateError('周期采集运行或状态未知时不能单次采集，请先停止');
    }
    final sensor = requestedSensor ?? selectedSensor;
    lastError = null;
    try {
      return await _captureOnceInternal(sensor);
    } catch (error) {
      _reportError('${sensor.label}单次采集失败', error);
      rethrow;
    } finally {
      measuringSensor = null;
      notifyListeners();
    }
  }

  Future<SingleMeasurement> _captureOnceInternal(SensorType sensor) async {
    measuringSensor = sensor;
    status = '正在单次采集${sensor.label}…';
    notifyListeners();
    final response = await _sendAndWait(
      DeviceCommands.captureOnce(
        sensor,
        spectrumGainCode: spectrumGainCode,
        flickerGainCode: flickerGainCode,
        ultravioletGainCode: ultravioletGainCode,
        proximityGainCode: proximityGainCode,
      ),
      expectedResponse: DeviceCommands.singleResponseCommand(sensor),
      timeout: const Duration(seconds: 6),
    );
    final result = SensorDataParser.parseSingle(
      sensor,
      response.data,
      spectrumGainCode: spectrumGainCode,
      flickerGainCode: flickerGainCode,
      ultravioletGainCode: ultravioletGainCode,
      proximityGainCode: proximityGainCode,
    );
    singleResults[sensor] = result;
    staleSingleResults.remove(sensor);
    status = switch (sensor) {
      SensorType.spectrum => '多光谱单次完成：增益${spectrumGainLabel(spectrumGainCode)}',
      SensorType.flicker =>
        '多光谱FIFO单次完成：增益${spectrumGainLabel(flickerGainCode)}',
      SensorType.ultraviolet =>
        '紫外单次完成：增益${ultravioletGainLabel(ultravioletGainCode)}',
      SensorType.proximity =>
        '接近单次完成：A=1×，B=${1 << proximityGainCode}×${result is ProximitySingleMeasurement && (result.channelASaturated || result.channelBSaturated) ? '（检测到饱和）' : ''}',
      _ => '${sensor.label}单次采集完成',
    };
    _addLog(status);
    return result;
  }

  Future<void> startAllSensors() async {
    _requireConnected();
    if (busy) throw StateError('当前有任务正在执行');
    if (collectionStateUnknown) {
      throw StateError('设备周期采集状态未知，请先停止全部');
    }
    final missing = SensorType.values
        .where((sensor) => !activeSchedules.contains(sensor))
        .toList(growable: false);
    if (missing.isEmpty) {
      status = '五类传感器均已在周期采集';
      notifyListeners();
      return;
    }
    lastError = null;
    scheduleCommandActive = true;
    status = missing.length == SensorType.values.length
        ? '正在下发五类周期采集命令…'
        : '正在启动剩余${missing.length}类传感器…';
    notifyListeners();
    try {
      for (final sensor in missing) {
        await _prepareSensorGain(sensor);
        await _configureSchedule(sensor);
      }
      collectionStateUnknown = false;
      status = '五类传感器正在周期采集';
    } catch (error) {
      if (error is! ProtocolException) collectionStateUnknown = true;
      _reportError('启动采集失败', error);
      rethrow;
    } finally {
      measuringSensor = null;
      scheduleCommandActive = false;
      notifyListeners();
    }
  }

  Future<void> startSensor(SensorType sensor) async {
    _requireConnected();
    if (busy) throw StateError('当前有任务正在执行');
    if (collectionStateUnknown) {
      throw StateError('设备周期采集状态未知，请先在首页停止全部');
    }
    if (activeSchedules.contains(sensor)) {
      throw StateError('${sensor.label}周期采集已经启动');
    }
    lastError = null;
    scheduleCommandActive = true;
    status = '正在启动${sensor.label}周期采集…';
    notifyListeners();
    try {
      await _prepareSensorGain(sensor);
      await _configureSchedule(sensor);
      status = '${sensor.label}正在周期采集';
    } catch (error) {
      if (error is! ProtocolException) collectionStateUnknown = true;
      _reportError('${sensor.label}启动失败', error);
      rethrow;
    } finally {
      measuringSensor = null;
      scheduleCommandActive = false;
      notifyListeners();
    }
  }

  Future<void> _prepareSensorGain(SensorType sensor) async {
    if (sensor != SensorType.spectrum &&
        sensor != SensorType.flicker &&
        sensor != SensorType.ultraviolet &&
        sensor != SensorType.proximity) {
      return;
    }
    if (sensor == SensorType.proximity) {
      await _prepareProximityScheduleGain();
      return;
    }
    await _captureOnceInternal(sensor);
    measuringSensor = null;
    final gainDescription = switch (sensor) {
      SensorType.spectrum =>
        '代码$spectrumGainCode/${spectrumGainLabel(spectrumGainCode)}',
      SensorType.flicker =>
        '代码$flickerGainCode/${spectrumGainLabel(flickerGainCode)}',
      SensorType.ultraviolet =>
        '代码$ultravioletGainCode/${ultravioletGainLabel(ultravioletGainCode)}',
      SensorType.proximity => '代码$proximityGainCode/${1 << proximityGainCode}×',
      _ => '',
    };
    _addLog('已用${sensor.label}单次测量设置周期增益：$gainDescription');
  }

  Future<void> _prepareProximityScheduleGain() async {
    while (true) {
      final result = await _captureOnceInternal(
        SensorType.proximity,
      ) as ProximitySingleMeasurement;
      measuringSensor = null;
      if (result.channelAInvalid || result.channelBInvalid) {
        throw const ProtocolException('接近预测量返回0xFFFF无效值，未启动周期采集');
      }
      if (result.channelASaturated) {
        throw const ProtocolException('接近A通道在固定1×下仍饱和，请增加距离、降低反射或调整结构');
      }
      if (!result.channelBSaturated) {
        _addLog(
          '接近周期增益确认：代码$proximityGainCode/${1 << proximityGainCode}×，B=${result.channelB}',
        );
        return;
      }
      if (proximityGainCode == 0) {
        throw const ProtocolException('接近B通道在1×下仍饱和，请增加距离、降低反射或调整结构');
      }
      final previousCode = proximityGainCode;
      proximityGainCode--;
      _addLog(
        '接近B=${result.channelB}达到满量程$proximityMaxCount，增益从${1 << previousCode}×降为${1 << proximityGainCode}×后重测',
      );
      notifyListeners();
    }
  }

  Future<void> _configureSchedule(SensorType sensor) async {
    final response = await _sendAndWait(
      DeviceCommands.startPeriodic(sensor),
      expectedResponse: 0x4f,
    );
    if (response.data.length != 1 || response.data.first != 0) {
      throw ProtocolException(
        '${sensor.label}启动失败，状态码 ${response.data.isEmpty ? '--' : response.data.first}',
      );
    }
    activeSchedules.add(sensor);
    _addLog('${sensor.label}周期采集已启动，周期 ${sensor.periodSeconds} 秒');
    notifyListeners();
  }

  Future<void> stopAllSensors() async {
    _requireConnected();
    if (busy) throw StateError('上传期间不能发送停止命令');
    scheduleCommandActive = true;
    status = '正在停止采集并排空加速度 FIFO…';
    notifyListeners();
    try {
      await _sendAndWait(
        DeviceCommands.stopPeriodic(),
        expectedResponse: 0x53,
        timeout: const Duration(seconds: 8),
      );
      activeSchedules.clear();
      collectionStateUnknown = false;
      status = '采集已停止，可以上传数据';
      _addLog('设备确认停止采集');
    } catch (error) {
      _reportError('停止采集失败', error);
      rethrow;
    } finally {
      scheduleCommandActive = false;
      notifyListeners();
    }
  }

  Future<void> upload(SensorType sensor) async {
    _requireConnected();
    if (busy) throw StateError('已有命令或上传任务正在执行');
    if (running || collectionStateUnknown) {
      throw StateError('请先停止设备周期采集，再上传历史');
    }
    selectedSensor = sensor;
    records[sensor]!.clear();
    packetCounts[sensor] = 0;
    _uploadPacketCount = 0;
    uploadingSensor = sensor;
    lastError = null;
    status = '正在上传${sensor.label}数据…';
    notifyListeners();
    try {
      await _write(DeviceCommands.upload(sensor));
      _uploadQuietTimer?.cancel();
      _uploadQuietTimer = Timer(const Duration(seconds: 6), () {
        if (uploadingSensor == sensor && _uploadPacketCount == 0) {
          _finishUpload(error: '6 秒内未收到上传数据');
        }
      });
    } catch (error) {
      _finishUpload(error: '发送上传命令失败：$error');
      rethrow;
    }
  }

  void cancelUpload() {
    if (uploadingSensor == null) return;
    _finishUpload(error: '上传已由用户终止；设备端是否已清除数据取决于最后一包是否发送成功');
  }

  void clearRecords(SensorType sensor) {
    records[sensor]!.clear();
    packetCounts[sensor] = 0;
    notifyListeners();
  }

  Future<ProtocolFrame> _sendAndWait(
    Uint8List command, {
    required int expectedResponse,
    Duration timeout = const Duration(seconds: 4),
  }) async {
    if (commandExecutor case final executor?) {
      _addLog('TX ${ProtocolFrame.toHex(command)}');
      return executor(
        command,
        expectedResponse: expectedResponse,
        timeout: timeout,
      );
    }
    if (_responseCompleter != null) {
      throw StateError('上一条命令仍在等待设备响应');
    }
    final completer = Completer<ProtocolFrame>();
    _responseCompleter = completer;
    _expectedResponse = expectedResponse;
    try {
      await _write(command);
      return await completer.future.timeout(timeout);
    } finally {
      if (identical(_responseCompleter, completer)) {
        _responseCompleter = null;
        _expectedResponse = null;
      }
    }
  }

  Future<void> _write(Uint8List bytes) async {
    _requireConnected();
    _addLog('TX ${ProtocolFrame.toHex(bytes)}');
    await UniversalBle.write(
      connectedDevice!.deviceId,
      _serviceUuid!,
      _writeUuid!,
      bytes,
      withoutResponse: true,
    );
  }

  void _onNotification(Uint8List bytes) {
    _addLog('RX ${ProtocolFrame.toHex(bytes)}');
    try {
      final frame = ProtocolFrame.decode(bytes);
      if (_responseCompleter != null &&
          frame.command == _expectedResponse &&
          !_responseCompleter!.isCompleted) {
        _responseCompleter!.complete(frame);
        return;
      }
      if (frame.command == 0x51) {
        _handleUploadFrame(frame);
      } else {
        _addLog('收到未匹配响应 0x${hexByte(frame.command)}');
      }
    } catch (error) {
      lastError = '接收数据解析失败：$error';
      _addLog(lastError!);
      notifyListeners();
    }
  }

  void _handleUploadFrame(ProtocolFrame frame) {
    final sensor = uploadingSensor;
    if (sensor == null) {
      _addLog('忽略未请求的 0x51 上传包（协议中不含传感器类型）');
      return;
    }
    final packet = SensorDataParser.parseUpload(sensor, frame.data);
    records[sensor]!.addAll(packet.records);
    packetCounts[sensor] = (packetCounts[sensor] ?? 0) + 1;
    _uploadPacketCount++;
    status =
        '正在上传${sensor.label}：${records[sensor]!.length} 条 / $_uploadPacketCount 包';

    _uploadQuietTimer?.cancel();
    if (packet.records.length < sensor.maxRecordsPerPacket) {
      _finishUpload();
    } else {
      _uploadQuietTimer = Timer(const Duration(seconds: 2), _finishUpload);
      notifyListeners();
    }
  }

  void _finishUpload({String? error}) {
    _uploadQuietTimer?.cancel();
    final sensor = uploadingSensor;
    if (sensor == null) return;
    uploadingSensor = null;
    if (error == null) {
      status =
          '${sensor.label}上传完成：${records[sensor]!.length} 条，${packetCounts[sensor]} 包';
      _addLog(status);
    } else {
      lastError = error;
      status = '${sensor.label}上传未正常完成';
      _addLog(error);
    }
    notifyListeners();
  }

  void _requireConnected() {
    if (!connected) throw StateError('请先连接 BLE 设备');
  }

  static bool _matchesShortUuid(String uuid, String shortUuid) {
    final normalized = uuid.toLowerCase();
    return normalized == shortUuid ||
        normalized == '0000$shortUuid-0000-1000-8000-00805f9b34fb';
  }

  void _reportError(String prefix, Object error) {
    lastError = '$prefix：$error';
    status = prefix;
    _addLog(lastError!);
    notifyListeners();
  }

  void _addLog(String text) {
    final now = DateTime.now();
    final time =
        '${now.hour.toString().padLeft(2, '0')}:'
        '${now.minute.toString().padLeft(2, '0')}:'
        '${now.second.toString().padLeft(2, '0')}';
    logLines.insert(0, '[$time] $text');
    if (logLines.length > 200) logLines.removeRange(200, logLines.length);
  }

  @override
  void dispose() {
    _scanTimer?.cancel();
    _uploadQuietTimer?.cancel();
    _scanSubscription?.cancel();
    _availabilitySubscription?.cancel();
    _connectionSubscription?.cancel();
    _valueSubscription?.cancel();
    super.dispose();
  }
}
