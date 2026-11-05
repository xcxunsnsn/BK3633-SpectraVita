import 'dart:math' as math;
import 'dart:typed_data';

enum SensorType {
  spectrum(1, '光谱', 22, 13, 60),
  flicker(2, '多光谱 FIFO', 256, 1, 600),
  ultraviolet(3, '紫外线', 8, 36, 60),
  proximity(4, '接近', 4, 73, 600),
  acceleration(5, '加速度', 7, 41, 1);

  const SensorType(
    this.id,
    this.label,
    this.recordLength,
    this.maxRecordsPerPacket,
    this.periodSeconds,
  );

  final int id;
  final String label;
  final int recordLength;
  final int maxRecordsPerPacket;
  final int periodSeconds;
}

// WH4530A固定PS_TIME=7时，PS_DATA满量程为2047。
const proximityMaxCount = 2047;
const flickerMinSignalRange = 5;

const spectrumGainMultipliers = <double>[
  0.5,
  1,
  2,
  4,
  8,
  16,
  32,
  64,
  128,
  256,
  512,
];
const ultravioletGainMultipliers = <int>[
  1,
  2,
  4,
  8,
  16,
  32,
  64,
  128,
  256,
  512,
  1024,
  2048,
];

String spectrumGainLabel(int code) =>
    '${code == 0 ? '0.5' : spectrumGainMultipliers[code].toInt()}×';
String ultravioletGainLabel(int code) => '${ultravioletGainMultipliers[code]}×';

class ProtocolException implements Exception {
  const ProtocolException(this.message);
  final String message;

  @override
  String toString() => message;
}

class ProtocolFrame {
  const ProtocolFrame({required this.command, required this.data});

  final int command;
  final Uint8List data;

  static Uint8List encode(int command, [List<int> data = const []]) {
    final body = <int>[command & 0xff, ...data.map((e) => e & 0xff)];
    final checksum = body.fold<int>(0, (value, byte) => value ^ byte);
    return Uint8List.fromList([0xaa, ...body, checksum, 0xbb]);
  }

  static ProtocolFrame decode(List<int> bytes) {
    if (bytes.length < 4) {
      throw const ProtocolException('数据帧长度不足');
    }
    if (bytes.first != 0xaa || bytes.last != 0xbb) {
      throw const ProtocolException('帧头或帧尾错误');
    }
    final checksumIndex = bytes.length - 2;
    var expected = 0;
    for (var i = 1; i < checksumIndex; i++) {
      expected ^= bytes[i];
    }
    if (bytes[checksumIndex] != expected) {
      throw ProtocolException(
        '校验失败：收到 ${hexByte(bytes[checksumIndex])}，应为 ${hexByte(expected)}',
      );
    }
    return ProtocolFrame(
      command: bytes[1],
      data: Uint8List.fromList(bytes.sublist(2, checksumIndex)),
    );
  }

  static String toHex(List<int> bytes) => bytes.map(hexByte).join(' ');

  static String hexByte(int value) =>
      (value & 0xff).toRadixString(16).padLeft(2, '0').toUpperCase();
}

class DeviceCommands {
  const DeviceCommands._();

  static const spectrumAtime = 29;
  static const spectrumAstep = 599;
  static const spectrumGain = 10;
  static const flickerTime = 720;
  static const flickerGain = 5;
  static const ultravioletGain = 9;

  static int singleResponseCommand(SensorType sensor) =>
      0x43 + sensor.index * 2;

  static Uint8List captureOnce(
    SensorType sensor, {
    int spectrumGainCode = spectrumGain,
    int flickerGainCode = flickerGain,
    int ultravioletGainCode = ultravioletGain,
    int proximityGainCode = 3,
  }) {
    switch (sensor) {
      case SensorType.spectrum:
        if (spectrumGainCode < 0 ||
            spectrumGainCode >= spectrumGainMultipliers.length) {
          throw const ProtocolException('多光谱增益代码必须为0～10');
        }
        return ProtocolFrame.encode(0x42, [
          spectrumAtime,
          spectrumAstep & 0xff,
          spectrumAstep >> 8,
          spectrumGainCode,
        ]);
      case SensorType.flicker:
        if (flickerGainCode < 0 ||
            flickerGainCode >= spectrumGainMultipliers.length) {
          throw const ProtocolException('多光谱FIFO增益代码必须为0～10');
        }
        return ProtocolFrame.encode(0x44, [
          flickerTime & 0xff,
          flickerTime >> 8,
          flickerGainCode,
        ]);
      case SensorType.ultraviolet:
        if (ultravioletGainCode < 0 ||
            ultravioletGainCode >= ultravioletGainMultipliers.length) {
          throw const ProtocolException('紫外增益代码必须为0～11');
        }
        return ProtocolFrame.encode(0x46, [ultravioletGainCode]);
      case SensorType.proximity:
        if (proximityGainCode < 0 || proximityGainCode > 3) {
          throw const ProtocolException('接近增益代码必须为0～3');
        }
        return ProtocolFrame.encode(0x48, [proximityGainCode]);
      case SensorType.acceleration:
        return ProtocolFrame.encode(0x4a);
    }
  }

  static Uint8List startPeriodic(SensorType sensor, {int? unixSeconds}) {
    final timestamp =
        unixSeconds ??
        DateTime.now().millisecondsSinceEpoch ~/ Duration.millisecondsPerSecond;
    final data = ByteData(11)
      ..setUint32(0, timestamp, Endian.little)
      ..setUint16(4, sensor.periodSeconds, Endian.little);
    for (var i = 0; i < SensorType.values.length; i++) {
      data.setUint8(6 + i, SensorType.values[i] == sensor ? 1 : 0);
    }
    return ProtocolFrame.encode(0x4e, data.buffer.asUint8List());
  }

  static Uint8List stopPeriodic() => ProtocolFrame.encode(0x52);

  static Uint8List upload(SensorType sensor) =>
      ProtocolFrame.encode(0x50, [sensor.id]);
}

sealed class SingleMeasurement {
  const SingleMeasurement({
    required this.sensor,
    required this.capturedAt,
    required this.raw,
  });

  final SensorType sensor;
  final DateTime capturedAt;
  final Uint8List raw;
  Map<String, String> get displayValues;
}

class SpectrumSingleMeasurement extends SingleMeasurement {
  const SpectrumSingleMeasurement({
    required super.capturedAt,
    required super.raw,
    required this.gainCode,
    required this.channels,
  }) : super(sensor: SensorType.spectrum);

  final int gainCode;
  final List<int> channels;
  @override
  Map<String, String> get displayValues => {
    '请求增益': spectrumGainLabel(gainCode),
    for (var i = 0; i < channels.length; i++)
      '${SpectrumRecord.wavelengths[i]} raw': '${channels[i]} count',
  };
}

class FlickerSingleMeasurement extends SingleMeasurement {
  const FlickerSingleMeasurement({
    required super.capturedAt,
    required super.raw,
    required this.gainCode,
    required this.samples,
    required this.validSampleEstimate,
    required this.dominantFrequencyHz,
  }) : super(sensor: SensorType.flicker);

  final int gainCode;
  final List<int> samples;
  final int validSampleEstimate;
  final double? dominantFrequencyHz;
  int get signalRange =>
      samples.isEmpty ? 0 : samples.reduce(math.max) - samples.reduce(math.min);
  bool get signalTooWeak => signalRange < flickerMinSignalRange;
  @override
  Map<String, String> get displayValues => {
    '请求增益': spectrumGainLabel(gainCode),
    '估算有效点': '$validSampleEstimate 点',
    '采样间隔':
        '${(FlickerRecord.sampleIntervalSeconds * 1000).toStringAsFixed(4)} ms',
    '峰峰值': '$signalRange count',
    '信号状态': signalTooWeak ? '信号过弱' : '可分析',
    '估算主频': dominantFrequencyHz == null
        ? (signalTooWeak ? '--（信号过弱）' : '--')
        : '${dominantFrequencyHz!.toStringAsFixed(2)} Hz',
    '前8点': '${samples.take(8).join(', ')} count',
  };
}

class UvSingleMeasurement extends SingleMeasurement {
  const UvSingleMeasurement({
    required super.capturedAt,
    required super.raw,
    required this.gainCode,
    required this.uva,
    required this.uvb,
    required this.uvc,
    required this.temperatureC,
  }) : super(sensor: SensorType.ultraviolet);

  final int gainCode;
  final int uva;
  final int uvb;
  final int uvc;
  final double temperatureC;
  @override
  Map<String, String> get displayValues => {
    '请求增益': ultravioletGainLabel(gainCode),
    'UVA': '$uva µW/cm²',
    'UVB': '$uvb µW/cm²',
    'UVC': '$uvc µW/cm²',
    '温度': '${temperatureC.toStringAsFixed(1)} ℃',
  };
}

class ProximitySingleMeasurement extends SingleMeasurement {
  const ProximitySingleMeasurement({
    required super.capturedAt,
    required super.raw,
    required this.gainCode,
    required this.channelA,
    required this.channelB,
  }) : super(sensor: SensorType.proximity);

  final int gainCode;
  final int channelA;
  final int channelB;
  bool get channelAInvalid => channelA == 0xffff;
  bool get channelBInvalid => channelB == 0xffff;
  bool get channelASaturated =>
      !channelAInvalid && channelA >= proximityMaxCount;
  bool get channelBSaturated =>
      !channelBInvalid && channelB >= proximityMaxCount;
  @override
  Map<String, String> get displayValues => {
    'A（固定1×）': channelAInvalid
        ? '无效（0xFFFF）'
        : '$channelA count${channelASaturated ? '（饱和）' : ''}',
    'B（${1 << gainCode}×）': channelBInvalid
        ? '无效（0xFFFF）'
        : '$channelB count${channelBSaturated ? '（饱和）' : ''}',
  };
}

class AccelerationSingleMeasurement extends SingleMeasurement {
  const AccelerationSingleMeasurement({
    required super.capturedAt,
    required super.raw,
    required this.xRaw,
    required this.yRaw,
    required this.zRaw,
  }) : super(sensor: SensorType.acceleration);

  final int xRaw;
  final int yRaw;
  final int zRaw;
  double get xG => xRaw * 0.0039;
  double get yG => yRaw * 0.0039;
  double get zG => zRaw * 0.0039;
  @override
  Map<String, String> get displayValues => {
    'X':
        '$xRaw LSB / ${xG.toStringAsFixed(4)} g / '
        '${(xG * 9.80665).toStringAsFixed(4)} m/s²',
    'Y':
        '$yRaw LSB / ${yG.toStringAsFixed(4)} g / '
        '${(yG * 9.80665).toStringAsFixed(4)} m/s²',
    'Z':
        '$zRaw LSB / ${zG.toStringAsFixed(4)} g / '
        '${(zG * 9.80665).toStringAsFixed(4)} m/s²',
  };
}

sealed class ParsedRecord {
  const ParsedRecord({
    required this.packetTimestamp,
    required this.recordIndex,
    required this.raw,
  });

  final int packetTimestamp;
  final int recordIndex;
  final Uint8List raw;

  SensorType get sensor;
  List<String> get tableValues;
  List<Object?> get csvValues;
}

class SpectrumRecord extends ParsedRecord {
  const SpectrumRecord({
    required super.packetTimestamp,
    required super.recordIndex,
    required super.raw,
    required this.gainCode,
    required this.channels,
  });

  static const wavelengths = <String>[
    '415',
    '445',
    '480',
    '515',
    '555',
    '590',
    '630',
    '680',
    'Clear',
    'NIR',
  ];

  final int gainCode;
  final List<int> channels;

  double get gainMultiplier {
    return _gainMultiplier(gainCode);
  }

  List<double> get gainNormalized =>
      channels.map((value) => value / gainMultiplier).toList(growable: false);

  @override
  SensorType get sensor => SensorType.spectrum;

  @override
  List<String> get tableValues => [
    gainCode.toString(),
    '${gainMultiplier.toStringAsFixed(1)}×',
    ...channels.map((e) => e.toString()),
  ];

  @override
  List<Object?> get csvValues => [
    gainCode,
    gainMultiplier,
    ...channels,
    ...gainNormalized.map((e) => e.toStringAsFixed(4)),
  ];
}

class FlickerRecord extends ParsedRecord {
  const FlickerRecord({
    required super.packetTimestamp,
    required super.recordIndex,
    required super.raw,
    required this.samples,
    required this.validSampleEstimate,
    required this.dominantFrequencyHz,
  });

  static const fdTime = 720;
  static const sampleIntervalSeconds = fdTime * 2.78e-6;

  final List<int> samples;
  final int validSampleEstimate;
  final double? dominantFrequencyHz;
  int get signalRange =>
      samples.isEmpty ? 0 : samples.reduce(math.max) - samples.reduce(math.min);
  bool get signalTooWeak => signalRange < flickerMinSignalRange;

  @override
  SensorType get sensor => SensorType.flicker;

  @override
  List<String> get tableValues => [
    validSampleEstimate.toString(),
    dominantFrequencyHz == null
        ? (signalTooWeak ? '--（信号过弱）' : '--')
        : dominantFrequencyHz!.toStringAsFixed(2),
    samples.take(8).join(', '),
  ];

  @override
  List<Object?> get csvValues => [
    validSampleEstimate,
    dominantFrequencyHz?.toStringAsFixed(4) ?? '',
    ...samples,
  ];
}

class UvRecord extends ParsedRecord {
  const UvRecord({
    required super.packetTimestamp,
    required super.recordIndex,
    required super.raw,
    required this.uva,
    required this.uvb,
    required this.uvc,
    required this.temperatureC,
  });

  final int uva;
  final int uvb;
  final int uvc;
  final double temperatureC;

  @override
  SensorType get sensor => SensorType.ultraviolet;

  @override
  List<String> get tableValues => [
    uva.toString(),
    uvb.toString(),
    uvc.toString(),
    temperatureC.toStringAsFixed(1),
  ];

  @override
  List<Object?> get csvValues => [uva, uvb, uvc, temperatureC];
}

class ProximityRecord extends ParsedRecord {
  const ProximityRecord({
    required super.packetTimestamp,
    required super.recordIndex,
    required super.raw,
    required this.channelA,
    required this.channelB,
  });

  final int channelA;
  final int channelB;

  @override
  SensorType get sensor => SensorType.proximity;

  @override
  List<String> get tableValues => [channelA.toString(), channelB.toString()];

  @override
  List<Object?> get csvValues => [channelA, channelB];
}

class AccelerationRecord extends ParsedRecord {
  const AccelerationRecord({
    required super.packetTimestamp,
    required super.recordIndex,
    required super.raw,
    required this.highSpeed,
    required this.xRaw,
    required this.yRaw,
    required this.zRaw,
    required this.estimatedTimestamp,
  });

  final bool highSpeed;
  final int xRaw;
  final int yRaw;
  final int zRaw;
  final double estimatedTimestamp;

  double get xG => xRaw * 0.0039;
  double get yG => yRaw * 0.0039;
  double get zG => zRaw * 0.0039;
  double get xMs2 => xG * 9.80665;
  double get yMs2 => yG * 9.80665;
  double get zMs2 => zG * 9.80665;

  @override
  SensorType get sensor => SensorType.acceleration;

  @override
  List<String> get tableValues => [
    highSpeed ? 'HIGH 50 Hz' : 'LOW 1 Hz',
    xRaw.toString(),
    yRaw.toString(),
    zRaw.toString(),
    xG.toStringAsFixed(4),
    yG.toStringAsFixed(4),
    zG.toStringAsFixed(4),
    xMs2.toStringAsFixed(4),
    yMs2.toStringAsFixed(4),
    zMs2.toStringAsFixed(4),
  ];

  @override
  List<Object?> get csvValues => [
    highSpeed ? 'HIGH' : 'LOW',
    xRaw,
    yRaw,
    zRaw,
    xG,
    yG,
    zG,
    xMs2,
    yMs2,
    zMs2,
    estimatedTimestamp.toStringAsFixed(3),
  ];
}

class UploadPacket {
  const UploadPacket({required this.timestamp, required this.records});
  final int timestamp;
  final List<ParsedRecord> records;
}

class SensorDataParser {
  const SensorDataParser._();

  static SingleMeasurement parseSingle(
    SensorType sensor,
    Uint8List raw, {
    int spectrumGainCode = DeviceCommands.spectrumGain,
    int flickerGainCode = DeviceCommands.flickerGain,
    int ultravioletGainCode = DeviceCommands.ultravioletGain,
    int proximityGainCode = 3,
    DateTime? capturedAt,
  }) {
    const lengths = <SensorType, int>{
      SensorType.spectrum: 20,
      SensorType.flicker: 256,
      SensorType.ultraviolet: 8,
      SensorType.proximity: 4,
      SensorType.acceleration: 6,
    };
    final expected = lengths[sensor]!;
    if (raw.length != expected) {
      throw ProtocolException(
        '${sensor.label}单次响应长度错误：${raw.length}，应为$expected',
      );
    }
    final time = capturedAt ?? DateTime.now();
    final data = ByteData.sublistView(raw);
    switch (sensor) {
      case SensorType.spectrum:
        return SpectrumSingleMeasurement(
          capturedAt: time,
          raw: raw,
          gainCode: spectrumGainCode,
          channels: List<int>.generate(
            10,
            (i) => data.getUint16(i * 2, Endian.little),
            growable: false,
          ),
        );
      case SensorType.flicker:
        final samples = _flickerSamples(raw);
        final valid = samples.lastIndexWhere((value) => value != 0) + 1;
        return FlickerSingleMeasurement(
          capturedAt: time,
          raw: raw,
          gainCode: flickerGainCode,
          samples: samples,
          validSampleEstimate: valid,
          dominantFrequencyHz: _dominantFrequency(samples.take(valid).toList()),
        );
      case SensorType.ultraviolet:
        return UvSingleMeasurement(
          capturedAt: time,
          raw: raw,
          gainCode: ultravioletGainCode,
          uva: data.getUint16(0, Endian.little),
          uvb: data.getUint16(2, Endian.little),
          uvc: data.getUint16(4, Endian.little),
          temperatureC: data.getInt16(6, Endian.little) / 10.0,
        );
      case SensorType.proximity:
        return ProximitySingleMeasurement(
          capturedAt: time,
          raw: raw,
          gainCode: proximityGainCode,
          channelA: data.getUint16(0, Endian.little),
          channelB: data.getUint16(2, Endian.little),
        );
      case SensorType.acceleration:
        return AccelerationSingleMeasurement(
          capturedAt: time,
          raw: raw,
          xRaw: data.getInt16(0, Endian.little),
          yRaw: data.getInt16(2, Endian.little),
          zRaw: data.getInt16(4, Endian.little),
        );
    }
  }

  static UploadPacket parseUpload(SensorType sensor, Uint8List data) {
    if (data.length < 4) {
      throw const ProtocolException('上传数据缺少首条记录时间戳');
    }
    final payloadLength = data.length - 4;
    if (payloadLength % sensor.recordLength != 0) {
      throw ProtocolException(
        '${sensor.label}数据长度错误：$payloadLength 不是记录长度 ${sensor.recordLength} 的整数倍',
      );
    }
    final count = payloadLength ~/ sensor.recordLength;
    if (count > sensor.maxRecordsPerPacket) {
      throw ProtocolException('${sensor.label}单包记录数 $count 超过上限');
    }
    final bytes = ByteData.sublistView(data);
    final timestamp = bytes.getUint32(0, Endian.little);
    final records = <ParsedRecord>[];
    var accTime = timestamp.toDouble();
    for (var index = 0; index < count; index++) {
      final offset = 4 + index * sensor.recordLength;
      final raw = Uint8List.fromList(
        data.sublist(offset, offset + sensor.recordLength),
      );
      final record = _parseRecord(sensor, timestamp, index, raw, accTime);
      records.add(record);
      if (record is AccelerationRecord) {
        accTime += record.highSpeed ? 0.02 : 1.0;
      }
    }
    return UploadPacket(timestamp: timestamp, records: records);
  }

  static ParsedRecord _parseRecord(
    SensorType sensor,
    int timestamp,
    int index,
    Uint8List raw,
    double accTime,
  ) {
    final data = ByteData.sublistView(raw);
    switch (sensor) {
      case SensorType.spectrum:
        return SpectrumRecord(
          packetTimestamp: timestamp,
          recordIndex: index,
          raw: raw,
          gainCode: data.getUint16(0, Endian.little),
          channels: List<int>.generate(
            10,
            (i) => data.getUint16(2 + i * 2, Endian.little),
            growable: false,
          ),
        );
      case SensorType.flicker:
        final samples = _flickerSamples(raw);
        final lastNonZero = samples.lastIndexWhere((value) => value != 0);
        final valid = lastNonZero + 1;
        return FlickerRecord(
          packetTimestamp: timestamp,
          recordIndex: index,
          raw: raw,
          samples: samples,
          validSampleEstimate: valid,
          dominantFrequencyHz: _dominantFrequency(samples.take(valid).toList()),
        );
      case SensorType.ultraviolet:
        return UvRecord(
          packetTimestamp: timestamp,
          recordIndex: index,
          raw: raw,
          uva: data.getUint16(0, Endian.little),
          uvb: data.getUint16(2, Endian.little),
          uvc: data.getUint16(4, Endian.little),
          temperatureC: data.getInt16(6, Endian.little) / 10.0,
        );
      case SensorType.proximity:
        return ProximityRecord(
          packetTimestamp: timestamp,
          recordIndex: index,
          raw: raw,
          channelA: data.getUint16(0, Endian.little),
          channelB: data.getUint16(2, Endian.little),
        );
      case SensorType.acceleration:
        return AccelerationRecord(
          packetTimestamp: timestamp,
          recordIndex: index,
          raw: raw,
          highSpeed: data.getUint8(0) == 1,
          xRaw: data.getInt16(1, Endian.little),
          yRaw: data.getInt16(3, Endian.little),
          zRaw: data.getInt16(5, Endian.little),
          estimatedTimestamp: accTime,
        );
    }
  }

  static double? _dominantFrequency(List<int> samples) {
    if (samples.length < 4) return null;
    final signalRange = samples.reduce(math.max) - samples.reduce(math.min);
    if (signalRange < flickerMinSignalRange) return null;
    final mean = samples.reduce((a, b) => a + b) / samples.length;
    var bestBin = 0;
    var bestPower = 0.0;
    for (var k = 1; k <= samples.length ~/ 2; k++) {
      var real = 0.0;
      var imaginary = 0.0;
      for (var n = 0; n < samples.length; n++) {
        final angle = 2 * math.pi * k * n / samples.length;
        final centered = samples[n] - mean;
        real += centered * math.cos(angle);
        imaginary -= centered * math.sin(angle);
      }
      final power = real * real + imaginary * imaginary;
      if (power > bestPower) {
        bestPower = power;
        bestBin = k;
      }
    }
    if (bestBin == 0 || bestPower == 0) return null;
    return bestBin / (samples.length * FlickerRecord.sampleIntervalSeconds);
  }

  static List<int> _flickerSamples(Uint8List raw) {
    final data = ByteData.sublistView(raw);
    return List<int>.generate(
      128,
      (i) => data.getUint16(i * 2, Endian.little),
      growable: false,
    );
  }
}

double _gainMultiplier(int gainCode) {
  return gainCode >= 0 && gainCode < spectrumGainMultipliers.length
      ? spectrumGainMultipliers[gainCode]
      : 1;
}

String hexByte(int value) => ProtocolFrame.hexByte(value);
