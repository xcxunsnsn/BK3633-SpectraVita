import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:universal_ble/universal_ble.dart';

import '../protocol/spectravita_protocol.dart';
import '../services/ble_controller.dart';
import '../services/data_exporter.dart';

typedef CsvExporter = Future<CsvExportResult> Function(
  SensorType sensor,
  List<ParsedRecord> records,
);

class HomePage extends StatefulWidget {
  const HomePage({super.key, this.controller, this.csvExporter});
  final BleController? controller;
  final CsvExporter? csvExporter;

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  late final BleController controller;
  late final bool ownsController;
  bool actionBusy = false;
  SensorType? pageSensor;

  @override
  void initState() {
    super.initState();
    ownsController = widget.controller == null;
    controller = widget.controller ?? BleController();
  }

  @override
  void dispose() {
    if (ownsController) controller.dispose();
    super.dispose();
  }

  Future<void> _run(Future<void> Function() action) async {
    if (actionBusy) return;
    setState(() => actionBusy = true);
    try {
      await action();
    } catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(context)
            .showSnackBar(SnackBar(content: Text(error.toString())));
      }
    } finally {
      if (mounted) setState(() => actionBusy = false);
    }
  }

  Future<void> _showDevices() async {
    try {
      await controller.startScan();
    } catch (_) {
      return;
    }
    if (!mounted) return;
    final selected = await showDialog<BleDevice>(
      context: context,
      builder: (dialogContext) => AlertDialog(
        title: const Text('选择 BLE 设备'),
        content: SizedBox(
          width: 520,
          height: 420,
          child: AnimatedBuilder(
            animation: controller,
            builder: (context, _) {
              final devices = controller.devices;
              return Column(
                children: [
                  Row(
                    children: [
                      if (controller.scanning)
                        const SizedBox.square(
                          dimension: 18,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        ),
                      if (controller.scanning) const SizedBox(width: 10),
                      Expanded(
                        child: Text(
                          controller.scanning ? '扫描中（12 秒后自动停止）' : '扫描结束，可重新扫描',
                        ),
                      ),
                      TextButton.icon(
                        onPressed: controller.scanning
                            ? null
                            : () => controller.startScan(),
                        icon: const Icon(Icons.refresh),
                        label: const Text('重新扫描'),
                      ),
                    ],
                  ),
                  const Divider(),
                  Expanded(
                    child: devices.isEmpty
                        ? const Center(child: Text('暂未发现设备'))
                        : ListView.separated(
                            itemCount: devices.length,
                            separatorBuilder: (_, _) =>
                                const Divider(height: 1),
                            itemBuilder: (context, index) {
                              final device = devices[index];
                              final name = device.name?.trim();
                              return ListTile(
                                leading: const CircleAvatar(
                                  child: Icon(Icons.bluetooth),
                                ),
                                title: Text(
                                  name == null || name.isEmpty ? '未命名设备' : name,
                                ),
                                subtitle: Text(device.deviceId),
                                trailing: Text('${device.rssi ?? '--'} dBm'),
                                onTap: () =>
                                    Navigator.pop(dialogContext, device),
                              );
                            },
                          ),
                  ),
                ],
              );
            },
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(dialogContext),
            child: const Text('取消'),
          ),
        ],
      ),
    );
    await controller.stopScan();
    if (selected != null) await _run(() => controller.connect(selected));
  }

  void _selectPage(SensorType? sensor) {
    if (sensor != null) controller.selectSensor(sensor);
    setState(() => pageSensor = sensor);
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: controller,
      builder: (context, _) {
        final wide = MediaQuery.sizeOf(context).width >= 900;
        final pageTitle = pageSensor?.label ?? '首页';
        return Scaffold(
          appBar: AppBar(
            title: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text('SpectraVita'),
                Text(
                  '$pageTitle · BLE 传感器采集与解析',
                  style: const TextStyle(fontSize: 12),
                ),
              ],
            ),
            actions: [
              _ConnectionButton(
                connected: controller.connected,
                connecting: controller.connecting,
                onConnect: actionBusy ? null : _showDevices,
                onDisconnect: actionBusy
                    ? null
                    : () => _run(controller.disconnect),
              ),
              const SizedBox(width: 12),
            ],
          ),
          drawer: wide
              ? null
              : _AppDrawer(
                  selectedSensor: pageSensor,
                  controller: controller,
                  onSelected: (sensor) {
                    Navigator.pop(context);
                    _selectPage(sensor);
                  },
                ),
          body: SafeArea(
            child: LayoutBuilder(
              builder: (context, constraints) {
                final content = pageSensor == null
                    ? _HomeDashboard(
                        controller: controller,
                        actionBusy: actionBusy,
                        run: _run,
                      )
                    : _SensorDashboard(
                        controller: controller,
                        sensor: pageSensor!,
                        actionBusy: actionBusy,
                        run: _run,
                        exportCsv: widget.csvExporter ?? DataExporter.exportCsv,
                      );
                if (!wide) return content;
                return Row(
                  children: [
                    _AppNavigationRail(
                      selectedSensor: pageSensor,
                      controller: controller,
                      onSelected: _selectPage,
                    ),
                    const VerticalDivider(width: 1),
                    Expanded(child: content),
                  ],
                );
              },
            ),
          ),
        );
      },
    );
  }
}

class _AppDrawer extends StatelessWidget {
  const _AppDrawer({
    required this.selectedSensor,
    required this.controller,
    required this.onSelected,
  });

  final SensorType? selectedSensor;
  final BleController controller;
  final ValueChanged<SensorType?> onSelected;

  @override
  Widget build(BuildContext context) {
    return Drawer(
      child: SafeArea(
        child: ListView(
          padding: const EdgeInsets.all(12),
          children: _navigationTiles(
            context,
            selectedSensor: selectedSensor,
            controller: controller,
            onSelected: onSelected,
          ),
        ),
      ),
    );
  }
}

class _AppNavigationRail extends StatelessWidget {
  const _AppNavigationRail({
    required this.selectedSensor,
    required this.controller,
    required this.onSelected,
  });

  final SensorType? selectedSensor;
  final BleController controller;
  final ValueChanged<SensorType?> onSelected;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 205,
      child: ListView(
        padding: const EdgeInsets.all(12),
        children: _navigationTiles(
          context,
          selectedSensor: selectedSensor,
          controller: controller,
          onSelected: onSelected,
        ),
      ),
    );
  }
}

List<Widget> _navigationTiles(
  BuildContext context, {
  required SensorType? selectedSensor,
  required BleController controller,
  required ValueChanged<SensorType?> onSelected,
}) {
  Widget tile(
    SensorType? sensor,
    String label,
    IconData icon,
    String subtitle,
  ) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 6),
      child: ListTile(
        dense: true,
        selected: selectedSensor == sensor,
        selectedTileColor: Theme.of(context).colorScheme.secondaryContainer,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        leading: Icon(icon),
        title: Text(label),
        subtitle: Text(subtitle),
        onTap: () => onSelected(sensor),
      ),
    );
  }

  return [
    Text('页面导航', style: Theme.of(context).textTheme.titleSmall),
    const SizedBox(height: 8),
    tile(
      null,
      '首页',
      Icons.home_outlined,
      '${controller.activeSchedules.length}/5 类运行',
    ),
    for (final sensor in SensorType.values)
      tile(
        sensor,
        sensor.label,
        _sensorIcon(sensor),
        controller.activeSchedules.contains(sensor)
            ? '周期运行中'
            : '${controller.records[sensor]!.length} 条数据',
      ),
  ];
}

class _ConnectionButton extends StatelessWidget {
  const _ConnectionButton({
    required this.connected,
    required this.connecting,
    required this.onConnect,
    required this.onDisconnect,
  });

  final bool connected;
  final bool connecting;
  final VoidCallback? onConnect;
  final VoidCallback? onDisconnect;

  @override
  Widget build(BuildContext context) {
    if (connecting) {
      return const Padding(
        padding: EdgeInsets.all(12),
        child: SizedBox.square(
          dimension: 22,
          child: CircularProgressIndicator(strokeWidth: 2),
        ),
      );
    }
    return FilledButton.tonalIcon(
      onPressed: connected ? onDisconnect : onConnect,
      icon: Icon(
        connected ? Icons.bluetooth_connected : Icons.bluetooth_searching,
      ),
      label: Text(connected ? '断开' : '连接设备'),
    );
  }
}

class _HomeDashboard extends StatelessWidget {
  const _HomeDashboard({
    required this.controller,
    required this.actionBusy,
    required this.run,
  });

  final BleController controller;
  final bool actionBusy;
  final Future<void> Function(Future<void> Function()) run;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        _StatusCard(controller: controller),
        const SizedBox(height: 12),
        _HomeScheduleCard(
          controller: controller,
          actionBusy: actionBusy,
          run: run,
        ),
        const SizedBox(height: 12),
        _HomeUploadCard(
          controller: controller,
          actionBusy: actionBusy,
          run: run,
        ),
        const SizedBox(height: 12),
        _CommunicationLogCard(controller: controller),
      ],
    );
  }
}

class _SensorDashboard extends StatelessWidget {
  const _SensorDashboard({
    required this.controller,
    required this.sensor,
    required this.actionBusy,
    required this.run,
    required this.exportCsv,
  });

  final BleController controller;
  final SensorType sensor;
  final bool actionBusy;
  final Future<void> Function(Future<void> Function()) run;
  final CsvExporter exportCsv;

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        _StatusCard(controller: controller),
        const SizedBox(height: 12),
        _SensorControlCard(
          controller: controller,
          sensor: sensor,
          actionBusy: actionBusy,
          run: run,
        ),
        const SizedBox(height: 12),
        _SingleMeasurementCard(controller: controller, sensor: sensor),
        const SizedBox(height: 12),
        _SensorDataCard(
          controller: controller,
          sensor: sensor,
          actionBusy: actionBusy,
          run: run,
          exportCsv: exportCsv,
        ),
      ],
    );
  }
}

class _SensorDataCard extends StatelessWidget {
  const _SensorDataCard({
    required this.controller,
    required this.sensor,
    required this.actionBusy,
    required this.run,
    required this.exportCsv,
  });

  final BleController controller;
  final SensorType sensor;
  final bool actionBusy;
  final Future<void> Function(Future<void> Function()) run;
  final CsvExporter exportCsv;

  @override
  Widget build(BuildContext context) {
    final records = controller.records[sensor]!;
    return _SectionCard(
      title: '${sensor.label}数据',
      trailing: Wrap(
        spacing: 8,
        children: [
          OutlinedButton.icon(
            onPressed: records.isEmpty || actionBusy
                ? null
                : () => run(() async {
                    final result = await exportCsv(sensor, records);
                    if (context.mounted) {
                      ScaffoldMessenger.of(
                        context,
                      ).showSnackBar(SnackBar(content: Text(result.message)));
                    }
                  }),
            icon: const Icon(Icons.download),
            label: const Text('导出 CSV'),
          ),
          IconButton(
            tooltip: '清空界面数据（不影响设备 Flash）',
            onPressed: records.isEmpty
                ? null
                : () => controller.clearRecords(sensor),
            icon: const Icon(Icons.delete_outline),
          ),
        ],
      ),
      child: records.isEmpty
          ? const SizedBox(height: 180, child: Center(child: Text('尚未上传该类数据')))
          : Column(
              children: [
                SizedBox(
                  height: 300,
                  child: SensorChart(sensor: sensor, records: records),
                ),
                const SizedBox(height: 12),
                _RecordTable(sensor: sensor, records: records),
              ],
            ),
    );
  }
}

class _CommunicationLogCard extends StatelessWidget {
  const _CommunicationLogCard({required this.controller});
  final BleController controller;

  @override
  Widget build(BuildContext context) {
    return _SectionCard(
      title: '通信日志（最近 200 条）',
      child: SizedBox(
        height: 220,
        child: controller.logLines.isEmpty
            ? const Center(child: Text('暂无通信记录'))
            : ListView.builder(
                itemCount: controller.logLines.length,
                itemBuilder: (context, index) => SelectableText(
                  controller.logLines[index],
                  style: const TextStyle(fontFamily: 'monospace', fontSize: 12),
                ),
              ),
      ),
    );
  }
}

class _StatusCard extends StatelessWidget {
  const _StatusCard({required this.controller});
  final BleController controller;

  @override
  Widget build(BuildContext context) {
    final colors = Theme.of(context).colorScheme;
    return Card(
      color: controller.lastError == null
          ? colors.primaryContainer
          : colors.errorContainer,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Icon(
              controller.lastError == null
                  ? Icons.info_outline
                  : Icons.error_outline,
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    controller.status,
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                  const SizedBox(height: 3),
                  Text(
                    controller.connectedDevice != null
                        ? 'MTU ${controller.negotiatedMtu ?? '系统自动'} · '
                              '${controller.connectedDevice!.name ?? controller.connectedDevice!.deviceId}'
                        : controller.connected
                        ? '命令通道已连接'
                        : '请连接提供 FEE0 / FEE2 / FEE4 的设备',
                  ),
                ],
              ),
            ),
            if (controller.uploadingSensor != null ||
                controller.measuringSensor != null ||
                controller.scheduleCommandActive)
              const SizedBox.square(
                dimension: 24,
                child: CircularProgressIndicator(strokeWidth: 3),
              ),
          ],
        ),
      ),
    );
  }
}

class _HomeScheduleCard extends StatelessWidget {
  const _HomeScheduleCard({
    required this.controller,
    required this.actionBusy,
    required this.run,
  });

  final BleController controller;
  final bool actionBusy;
  final Future<void> Function(Future<void> Function()) run;

  @override
  Widget build(BuildContext context) {
    final disabled = !controller.connected || actionBusy || controller.busy;
    final missingCount =
        SensorType.values.length - controller.activeSchedules.length;
    final startLabel = missingCount == 0
        ? '五类已全部启动'
        : controller.activeSchedules.isEmpty
        ? '启动五类周期采集'
        : '启动剩余$missingCount类';
    return _SectionCard(
      title: '五类周期采集',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Wrap(
            spacing: 10,
            runSpacing: 10,
            children: [
              FilledButton.icon(
                onPressed:
                    disabled ||
                        controller.collectionStateUnknown ||
                        missingCount == 0
                    ? null
                    : () => run(controller.startAllSensors),
                icon: const Icon(Icons.play_arrow),
                label: Text(startLabel),
              ),
              FilledButton.tonalIcon(
                onPressed:
                    disabled ||
                        (!controller.running &&
                            !controller.collectionStateUnknown)
                    ? null
                    : () => run(controller.stopAllSensors),
                icon: const Icon(Icons.stop),
                label: const Text('停止全部周期采集'),
              ),
            ],
          ),
          const SizedBox(height: 14),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: SensorType.values
                .map(
                  (sensor) => Chip(
                    avatar: Icon(
                      controller.activeSchedules.contains(sensor)
                          ? Icons.check_circle
                          : Icons.pause_circle_outline,
                      size: 18,
                    ),
                    label: Text(
                      '${sensor.label}：${controller.activeSchedules.contains(sensor) ? '运行中' : '未启动'}',
                    ),
                  ),
                )
                .toList(),
          ),
          if (controller.collectionStateUnknown) ...[
            const SizedBox(height: 8),
            Text(
              '连接曾中断，以上仅为最后已知状态。请先执行“停止全部周期采集”确认设备状态。',
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
          ],
          const SizedBox(height: 8),
          Text(
            '固定周期：光谱/紫外 60 s，多光谱FIFO/接近 600 s；加速度 LOW 1 Hz、HIGH 50 Hz。',
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ],
      ),
    );
  }
}

class _HomeUploadCard extends StatelessWidget {
  const _HomeUploadCard({
    required this.controller,
    required this.actionBusy,
    required this.run,
  });

  final BleController controller;
  final bool actionBusy;
  final Future<void> Function(Future<void> Function()) run;

  @override
  Widget build(BuildContext context) {
    final disabled =
        !controller.connected ||
        actionBusy ||
        controller.busy ||
        controller.running ||
        controller.collectionStateUnknown;
    return _SectionCard(
      title: '按类型依次上传',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text('上传前必须停止全部周期采集；一次只能上传一种传感器。'),
          const SizedBox(height: 10),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: SensorType.values.map((sensor) {
              final uploading = controller.uploadingSensor == sensor;
              return OutlinedButton.icon(
                onPressed: disabled
                    ? null
                    : () => run(() => controller.upload(sensor)),
                icon: uploading
                    ? const SizedBox.square(
                        dimension: 16,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : Icon(_sensorIcon(sensor)),
                label: Text('上传${sensor.label}'),
              );
            }).toList(),
          ),
          if (controller.uploadingSensor != null) ...[
            const SizedBox(height: 8),
            TextButton.icon(
              onPressed: controller.cancelUpload,
              icon: const Icon(Icons.cancel_outlined),
              label: const Text('停止等待上传'),
            ),
          ],
        ],
      ),
    );
  }
}

class _SensorControlCard extends StatelessWidget {
  const _SensorControlCard({
    required this.controller,
    required this.sensor,
    required this.actionBusy,
    required this.run,
  });

  final BleController controller;
  final SensorType sensor;
  final bool actionBusy;
  final Future<void> Function(Future<void> Function()) run;

  @override
  Widget build(BuildContext context) {
    final disabled = !controller.connected || actionBusy || controller.busy;
    final currentRunning = controller.activeSchedules.contains(sensor);
    final anyPeriodic = controller.running || controller.collectionStateUnknown;
    return _SectionCard(
      title: '${sensor.label}采集与上传',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Wrap(
            spacing: 10,
            runSpacing: 10,
            children: [
              FilledButton.tonalIcon(
                onPressed: disabled || anyPeriodic
                    ? null
                    : () => run(() async {
                        await controller.captureOnce(sensor);
                      }),
                icon: controller.measuringSensor == sensor
                    ? const SizedBox.square(
                        dimension: 18,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : Icon(_sensorIcon(sensor)),
                label: Text('单次采集${sensor.label}'),
              ),
              FilledButton.icon(
                onPressed:
                    disabled ||
                        controller.collectionStateUnknown ||
                        currentRunning
                    ? null
                    : () => run(() => controller.startSensor(sensor)),
                icon: const Icon(Icons.play_arrow),
                label: Text(currentRunning ? '周期采集中' : '启动当前传感器周期采集'),
              ),
              OutlinedButton.icon(
                onPressed: disabled || anyPeriodic
                    ? null
                    : () => run(() => controller.upload(sensor)),
                icon: controller.uploadingSensor == sensor
                    ? const SizedBox.square(
                        dimension: 16,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.cloud_download_outlined),
                label: Text('上传${sensor.label}数据'),
              ),
            ],
          ),
          if (_gainOptions(sensor) case final options?) ...[
            const SizedBox(height: 12),
            Wrap(
              crossAxisAlignment: WrapCrossAlignment.center,
              spacing: 8,
              children: [
                Text('${_gainControlLabel(sensor)}：'),
                DropdownButton<int>(
                  value: _selectedGainCode(controller, sensor),
                  onChanged:
                      disabled ||
                          controller.collectionStateUnknown ||
                          currentRunning
                      ? null
                      : (value) {
                          if (value != null) {
                            _setSelectedGainCode(controller, sensor, value);
                          }
                        },
                  items: [
                    for (var code = 0; code < options.length; code++)
                      DropdownMenuItem(value: code, child: Text(options[code])),
                  ],
                ),
                Text(_gainControlNote(sensor)),
              ],
            ),
          ],
          if (currentRunning || controller.collectionStateUnknown) ...[
            const SizedBox(height: 10),
            Text(
              controller.collectionStateUnknown
                  ? '设备周期状态未知，请返回首页停止全部。'
                  : '${sensor.label}周期采集正在运行；停止操作请返回首页。',
              style: TextStyle(color: Theme.of(context).colorScheme.primary),
            ),
          ],
          if (controller.uploadingSensor == sensor) ...[
            const SizedBox(height: 8),
            TextButton.icon(
              onPressed: controller.cancelUpload,
              icon: const Icon(Icons.cancel_outlined),
              label: const Text('停止等待上传'),
            ),
          ],
          const SizedBox(height: 8),
          Text(
            '周期：${sensor.periodSeconds} s${sensor == SensorType.acceleration ? '（LOW 1 Hz、HIGH 50 Hz）' : ''}',
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ],
      ),
    );
  }
}

class _SingleMeasurementCard extends StatelessWidget {
  const _SingleMeasurementCard({
    required this.controller,
    required this.sensor,
  });
  final BleController controller;
  final SensorType sensor;

  @override
  Widget build(BuildContext context) {
    final result = controller.singleResults[sensor];
    return _SectionCard(
      title: '${sensor.label}最近一次单次结果',
      trailing: controller.staleSingleResults.contains(sensor) && result != null
          ? const Chip(
              avatar: Icon(Icons.history, size: 16),
              label: Text('旧连接数据'),
            )
          : null,
      child: result == null
          ? const SizedBox(
              height: 72,
              child: Center(child: Text('尚未进行该传感器的单次采集')),
            )
          : Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  '采集时间：${DateFormat('MM-dd HH:mm:ss').format(result.capturedAt)}',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
                const SizedBox(height: 10),
                Wrap(
                  spacing: 10,
                  runSpacing: 10,
                  children: result.displayValues.entries
                      .map(
                        (entry) => Container(
                          constraints: const BoxConstraints(minWidth: 130),
                          padding: const EdgeInsets.symmetric(
                            horizontal: 12,
                            vertical: 10,
                          ),
                          decoration: BoxDecoration(
                            color: Theme.of(context)
                                .colorScheme
                                .surfaceContainerHighest,
                            borderRadius: BorderRadius.circular(10),
                          ),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                entry.key,
                                style: Theme.of(context).textTheme.labelMedium,
                              ),
                              const SizedBox(height: 3),
                              SelectableText(entry.value),
                            ],
                          ),
                        ),
                      )
                      .toList(),
                ),
                const SizedBox(height: 8),
                SelectableText(
                  '原始HEX：${ProtocolFrame.toHex(result.raw)}',
                  style: const TextStyle(fontFamily: 'monospace', fontSize: 11),
                ),
              ],
            ),
    );
  }
}

class _SectionCard extends StatelessWidget {
  const _SectionCard({required this.title, required this.child, this.trailing});
  final String title;
  final Widget child;
  final Widget? trailing;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Row(
              children: [
                Expanded(
                  child: Text(
                    title,
                    style: Theme.of(context).textTheme.titleLarge,
                  ),
                ),
                ?trailing,
              ],
            ),
            const SizedBox(height: 12),
            child,
          ],
        ),
      ),
    );
  }
}

class _RecordTable extends StatelessWidget {
  const _RecordTable({required this.sensor, required this.records});
  final SensorType sensor;
  final List<ParsedRecord> records;

  @override
  Widget build(BuildContext context) {
    final visible = records.length <= 200
        ? records
        : records.sublist(records.length - 200);
    final headers = _tableHeaders(sensor);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        if (records.length > 200)
          Padding(
            padding: const EdgeInsets.only(bottom: 6),
            child: Text('表格仅显示最后 200 条；CSV 会导出全部 ${records.length} 条。'),
          ),
        if (sensor == SensorType.proximity)
          const Padding(
            padding: EdgeInsets.only(bottom: 6),
            child: Text('接近A固定1×，B使用命令增益；记录未携带B的增益代码，原始值不能直接当作毫米距离。'),
          ),
        if (sensor == SensorType.spectrum)
          const Padding(
            padding: EdgeInsets.only(bottom: 6),
            child: Text('光谱 normalized 仅除以增益倍数，未经标定，不能作为绝对辐照度。'),
          ),
        SingleChildScrollView(
          scrollDirection: Axis.horizontal,
          child: DataTable(
            headingRowHeight: 44,
            dataRowMinHeight: 38,
            dataRowMaxHeight: 48,
            columns: [
              const DataColumn(label: Text('时间（本地）')),
              ...headers.map((e) => DataColumn(label: Text(e))),
              const DataColumn(label: Text('原始 HEX')),
            ],
            rows: visible.reversed.map((record) {
              return DataRow(
                cells: [
                  DataCell(Text(_recordTime(record))),
                  ...record.tableValues.map((e) => DataCell(Text(e))),
                  DataCell(
                    SelectableText(
                      ProtocolFrame.toHex(record.raw),
                      style: const TextStyle(
                        fontFamily: 'monospace',
                        fontSize: 11,
                      ),
                    ),
                  ),
                ],
              );
            }).toList(),
          ),
        ),
      ],
    );
  }
}

class SensorChart extends StatelessWidget {
  const SensorChart({super.key, required this.sensor, required this.records});
  final SensorType sensor;
  final List<ParsedRecord> records;

  @override
  Widget build(BuildContext context) {
    if (sensor == SensorType.spectrum) {
      final record = records.last as SpectrumRecord;
      return BarChart(
        BarChartData(
          alignment: BarChartAlignment.spaceAround,
          borderData: FlBorderData(show: false),
          gridData: const FlGridData(show: true),
          titlesData: FlTitlesData(
            topTitles: const AxisTitles(
              sideTitles: SideTitles(showTitles: false),
            ),
            rightTitles: const AxisTitles(
              sideTitles: SideTitles(showTitles: false),
            ),
            leftTitles: const AxisTitles(
              axisNameWidget: Text(
                '原始计数（count）',
                style: TextStyle(fontSize: 11),
              ),
              axisNameSize: 24,
              sideTitles: SideTitles(showTitles: true, reservedSize: 46),
            ),
            bottomTitles: AxisTitles(
              axisNameWidget: const Text(
                '波长/通道（nm）',
                style: TextStyle(fontSize: 11),
              ),
              axisNameSize: 24,
              sideTitles: SideTitles(
                showTitles: true,
                reservedSize: 38,
                getTitlesWidget: (value, meta) {
                  final index = value.toInt();
                  if (index < 0 || index >= SpectrumRecord.wavelengths.length) {
                    return const SizedBox.shrink();
                  }
                  return Padding(
                    padding: const EdgeInsets.only(top: 6),
                    child: Text(
                      SpectrumRecord.wavelengths[index],
                      style: const TextStyle(fontSize: 10),
                    ),
                  );
                },
              ),
            ),
          ),
          barGroups: List.generate(
            record.channels.length,
            (i) => BarChartGroupData(
              x: i,
              barRods: [
                BarChartRodData(toY: record.channels[i].toDouble(), width: 16),
              ],
            ),
          ),
        ),
      );
    }

    final series = _chartSeries(sensor, records);
    final labels = _chartAxisLabels(sensor);
    return LineChart(
      LineChartData(
        borderData: FlBorderData(show: false),
        gridData: const FlGridData(show: true),
        titlesData: FlTitlesData(
          topTitles: const AxisTitles(
            sideTitles: SideTitles(showTitles: false),
          ),
          rightTitles: const AxisTitles(
            sideTitles: SideTitles(showTitles: false),
          ),
          leftTitles: AxisTitles(
            axisNameWidget: Text(
              labels.y,
              style: const TextStyle(fontSize: 11),
            ),
            axisNameSize: 25,
            sideTitles: const SideTitles(showTitles: true, reservedSize: 48),
          ),
          bottomTitles: AxisTitles(
            axisNameWidget: Text(
              labels.x,
              style: const TextStyle(fontSize: 11),
            ),
            axisNameSize: 24,
            sideTitles: SideTitles(
              showTitles: true,
              reservedSize: 30,
              getTitlesWidget: (value, meta) => Text(
                _formatAxisValue(value),
                style: const TextStyle(fontSize: 9),
              ),
            ),
          ),
        ),
        lineBarsData: List.generate(series.length, (index) {
          final colors = [Colors.teal, Colors.orange, Colors.purple];
          return LineChartBarData(
            spots: series[index],
            color: colors[index % colors.length],
            dotData: const FlDotData(show: false),
            barWidth: 2,
            isCurved: false,
          );
        }),
      ),
    );
  }
}

List<List<FlSpot>> _chartSeries(SensorType sensor, List<ParsedRecord> records) {
  if (sensor == SensorType.flicker) {
    final samples = (records.last as FlickerRecord).samples;
    return [
      List.generate(
        samples.length,
        (i) => FlSpot(
          i * FlickerRecord.sampleIntervalSeconds * 1000,
          samples[i].toDouble(),
        ),
      ),
    ];
  }
  final start = records.length > 500 ? records.length - 500 : 0;
  final recent = records.sublist(start);
  switch (sensor) {
    case SensorType.ultraviolet:
      return List.generate(
        3,
        (series) => List.generate(recent.length, (i) {
          final r = recent[i] as UvRecord;
          return FlSpot(
            (start + i) * sensor.periodSeconds / 60,
            [r.uva, r.uvb, r.uvc][series].toDouble(),
          );
        }),
      );
    case SensorType.proximity:
      return List.generate(
        2,
        (series) => List.generate(recent.length, (i) {
          final r = recent[i] as ProximityRecord;
          return FlSpot(
            (start + i) * sensor.periodSeconds / 60,
            [r.channelA, r.channelB][series].toDouble(),
          );
        }),
      );
    case SensorType.acceleration:
      final firstTime = (recent.first as AccelerationRecord).estimatedTimestamp;
      return List.generate(
        3,
        (series) => List.generate(recent.length, (i) {
          final r = recent[i] as AccelerationRecord;
          return FlSpot(
            r.estimatedTimestamp - firstTime,
            [r.xG, r.yG, r.zG][series],
          );
        }),
      );
    case SensorType.spectrum:
    case SensorType.flicker:
      return const [];
  }
}

List<String> _tableHeaders(SensorType sensor) {
  switch (sensor) {
    case SensorType.spectrum:
      return [
        '增益码',
        '增益',
        ...SpectrumRecord.wavelengths.map((e) => '$e raw (count)'),
      ];
    case SensorType.flicker:
      return ['估算有效点 (点)', '主频 (Hz)', '前8点 (count)'];
    case SensorType.ultraviolet:
      return ['UVA µW/cm²', 'UVB µW/cm²', 'UVC µW/cm²', '温度 ℃'];
    case SensorType.proximity:
      return ['A原始计数 (1×)', 'B原始计数 (命令增益)'];
    case SensorType.acceleration:
      return [
        '模式',
        'X raw (LSB)',
        'Y raw (LSB)',
        'Z raw (LSB)',
        'X (g)',
        'Y (g)',
        'Z (g)',
        'X (m/s²)',
        'Y (m/s²)',
        'Z (m/s²)',
      ];
  }
}

({String x, String y}) _chartAxisLabels(SensorType sensor) {
  switch (sensor) {
    case SensorType.spectrum:
      return (x: '波长/通道（nm）', y: '原始计数（count）');
    case SensorType.flicker:
      return (x: '单次FIFO内相对时间（ms）', y: 'Flicker ADC原始计数（count）');
    case SensorType.ultraviolet:
      return (x: '相对时间（min）', y: '辐照度（µW/cm²）');
    case SensorType.proximity:
      return (x: '相对时间（min）', y: '原始计数（count）');
    case SensorType.acceleration:
      return (x: '相对时间（s）', y: '加速度（g）');
  }
}

String _formatAxisValue(double value) {
  final absolute = value.abs();
  if (absolute >= 1000) return value.toStringAsFixed(0);
  if (absolute >= 10) return value.toStringAsFixed(1);
  return value.toStringAsFixed(2);
}

List<String>? _gainOptions(SensorType sensor) {
  return switch (sensor) {
    SensorType.spectrum => List.generate(
      spectrumGainMultipliers.length,
      spectrumGainLabel,
      growable: false,
    ),
    SensorType.flicker => List.generate(
      spectrumGainMultipliers.length,
      spectrumGainLabel,
      growable: false,
    ),
    SensorType.ultraviolet => List.generate(
      ultravioletGainMultipliers.length,
      ultravioletGainLabel,
      growable: false,
    ),
    SensorType.proximity => const ['1×', '2×', '4×', '8×'],
    _ => null,
  };
}

String _gainControlLabel(SensorType sensor) {
  return switch (sensor) {
    SensorType.spectrum => '多光谱增益',
    SensorType.flicker => '多光谱FIFO增益',
    SensorType.ultraviolet => '紫外增益',
    SensorType.proximity => '接近B通道增益',
    _ => '',
  };
}

String _gainControlNote(SensorType sensor) {
  return switch (sensor) {
    SensorType.spectrum => 'ATIME=29，ASTEP=599',
    SensorType.flicker => 'FD_TIME=720，低于5 count时不估算主频',
    SensorType.ultraviolet => '周期结果按增益换算为µW/cm²',
    SensorType.proximity => 'A通道始终为1×',
    _ => '',
  };
}

int _selectedGainCode(BleController controller, SensorType sensor) {
  return switch (sensor) {
    SensorType.spectrum => controller.spectrumGainCode,
    SensorType.flicker => controller.flickerGainCode,
    SensorType.ultraviolet => controller.ultravioletGainCode,
    SensorType.proximity => controller.proximityGainCode,
    _ => 0,
  };
}

void _setSelectedGainCode(
  BleController controller,
  SensorType sensor,
  int code,
) {
  switch (sensor) {
    case SensorType.spectrum:
      controller.setSpectrumGainCode(code);
      break;
    case SensorType.ultraviolet:
      controller.setUltravioletGainCode(code);
      break;
    case SensorType.flicker:
      controller.setFlickerGainCode(code);
      break;
    case SensorType.proximity:
      controller.setProximityGainCode(code);
      break;
    case SensorType.acceleration:
      break;
  }
}

String _recordTime(ParsedRecord record) {
  final seconds = record is AccelerationRecord
      ? record.estimatedTimestamp
      : record.packetTimestamp +
            record.recordIndex * record.sensor.periodSeconds;
  final date = DateTime.fromMillisecondsSinceEpoch(
    (seconds * 1000).round(),
    isUtc: true,
  ).toLocal();
  final format = record is AccelerationRecord && record.highSpeed
      ? DateFormat('MM-dd HH:mm:ss.SSS')
      : DateFormat('MM-dd HH:mm:ss');
  return format.format(date);
}

IconData _sensorIcon(SensorType sensor) {
  switch (sensor) {
    case SensorType.spectrum:
      return Icons.gradient;
    case SensorType.flicker:
      return Icons.waves;
    case SensorType.ultraviolet:
      return Icons.wb_sunny_outlined;
    case SensorType.proximity:
      return Icons.sensors;
    case SensorType.acceleration:
      return Icons.vibration;
  }
}
