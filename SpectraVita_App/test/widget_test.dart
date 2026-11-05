import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:spectravita_app/protocol/spectravita_protocol.dart';
import 'package:spectravita_app/screens/home_page.dart';
import 'package:spectravita_app/services/ble_controller.dart';
import 'package:spectravita_app/services/data_exporter.dart';

Future<ProtocolFrame> _successResponse(
  Uint8List command, {
  required int expectedResponse,
  required Duration timeout,
}) async {
  if (expectedResponse == 0x43) {
    return ProtocolFrame(command: 0x43, data: Uint8List(20));
  }
  if (expectedResponse == 0x45) {
    return ProtocolFrame(command: 0x45, data: Uint8List(256));
  }
  if (expectedResponse == 0x47) {
    return ProtocolFrame(command: 0x47, data: Uint8List(8));
  }
  if (expectedResponse == 0x49) {
    return ProtocolFrame(command: 0x49, data: Uint8List(4));
  }
  if (expectedResponse == 0x4f) {
    return ProtocolFrame(command: 0x4f, data: Uint8List.fromList([0]));
  }
  if (expectedResponse == 0x53) {
    return ProtocolFrame(command: 0x53, data: Uint8List(0));
  }
  throw StateError('Unexpected command');
}

void main() {
  testWidgets('mobile drawer opens home and proximity page controls', (
    tester,
  ) async {
    await tester.binding.setSurfaceSize(const Size(430, 900));
    final controller = BleController(
      initializeBle: false,
      commandExecutor: _successResponse,
    );
    await tester.pumpWidget(
      MaterialApp(home: HomePage(controller: controller)),
    );

    expect(find.text('五类周期采集'), findsOneWidget);
    expect(find.text('按类型依次上传'), findsOneWidget);
    expect(find.text('单次采集光谱'), findsNothing);
    for (final sensor in SensorType.values) {
      expect(
        find.widgetWithText(OutlinedButton, '上传${sensor.label}'),
        findsOneWidget,
      );
    }

    await tester.tap(find.byIcon(Icons.menu));
    await tester.pumpAndSettle();
    expect(find.byType(Drawer), findsOneWidget);
    await tester.tap(find.widgetWithText(ListTile, '接近'));
    await tester.pumpAndSettle();

    expect(find.text('单次采集接近'), findsOneWidget);
    expect(find.text('启动当前传感器周期采集'), findsOneWidget);
    expect(find.text('上传接近数据'), findsOneWidget);
    expect(find.widgetWithText(OutlinedButton, '上传光谱'), findsNothing);
    final gain = tester.widget<DropdownButton<int>>(
      find.byType(DropdownButton<int>),
    );
    expect(gain.value, 3);
    expect(gain.onChanged, isNotNull);

    await controller.startSensor(SensorType.spectrum);
    await tester.pump();
    final singleButton = tester.widget<FilledButton>(
      find.widgetWithText(FilledButton, '单次采集接近'),
    );
    final uploadButton = tester.widget<OutlinedButton>(
      find.widgetWithText(OutlinedButton, '上传接近数据'),
    );
    final addScheduleButton = tester.widget<FilledButton>(
      find.widgetWithText(FilledButton, '启动当前传感器周期采集'),
    );
    expect(singleButton.onPressed, isNull);
    expect(uploadButton.onPressed, isNull);
    expect(addScheduleButton.onPressed, isNotNull);
    expect(
      tester
          .widget<DropdownButton<int>>(find.byType(DropdownButton<int>))
          .onChanged,
      isNotNull,
    );

    await tester.pumpWidget(const SizedBox.shrink());
    controller.dispose();
    await tester.binding.setSurfaceSize(null);
  });

  testWidgets('mobile spectrum, FIFO and UV pages show gain defaults', (
    tester,
  ) async {
    await tester.binding.setSurfaceSize(const Size(430, 900));
    final controller = BleController(
      initializeBle: false,
      commandExecutor: _successResponse,
    );
    await tester.pumpWidget(
      MaterialApp(home: HomePage(controller: controller)),
    );

    await tester.tap(find.byIcon(Icons.menu));
    await tester.pumpAndSettle();
    await tester.tap(find.widgetWithText(ListTile, '光谱'));
    await tester.pumpAndSettle();
    var gain = tester.widget<DropdownButton<int>>(
      find.byType(DropdownButton<int>),
    );
    expect(find.text('多光谱增益：'), findsOneWidget);
    expect(gain.value, 10);
    expect(gain.items, hasLength(11));
    gain.onChanged!(0);
    await tester.pump();
    expect(controller.spectrumGainCode, 0);

    await tester.tap(find.byIcon(Icons.menu));
    await tester.pumpAndSettle();
    await tester.tap(find.widgetWithText(ListTile, '多光谱 FIFO'));
    await tester.pumpAndSettle();
    gain = tester.widget<DropdownButton<int>>(find.byType(DropdownButton<int>));
    expect(find.text('多光谱FIFO增益：'), findsOneWidget);
    expect(gain.value, 5);
    expect(gain.items, hasLength(11));
    gain.onChanged!(10);
    await tester.pump();
    expect(controller.flickerGainCode, 10);

    await tester.tap(find.byIcon(Icons.menu));
    await tester.pumpAndSettle();
    await tester.tap(find.widgetWithText(ListTile, '紫外线'));
    await tester.pumpAndSettle();
    gain = tester.widget<DropdownButton<int>>(find.byType(DropdownButton<int>));
    expect(find.text('紫外增益：'), findsOneWidget);
    expect(gain.value, 9);
    expect(gain.items, hasLength(12));
    gain.onChanged!(11);
    await tester.pump();
    expect(controller.ultravioletGainCode, 11);

    await tester.pumpWidget(const SizedBox.shrink());
    controller.dispose();
    await tester.binding.setSurfaceSize(null);
  });

  testWidgets('unknown collection state only allows stop all on home', (
    tester,
  ) async {
    await tester.binding.setSurfaceSize(const Size(800, 900));
    final controller = BleController(
      initializeBle: false,
      commandExecutor: _successResponse,
    );
    controller.activeSchedules.add(SensorType.spectrum);
    controller.collectionStateUnknown = true;
    await tester.pumpWidget(
      MaterialApp(home: HomePage(controller: controller)),
    );

    final start = tester.widget<FilledButton>(
      find.widgetWithText(FilledButton, '启动剩余4类'),
    );
    final stop = tester.widget<FilledButton>(
      find.widgetWithText(FilledButton, '停止全部周期采集'),
    );
    final upload = tester.widget<OutlinedButton>(
      find.widgetWithText(OutlinedButton, '上传光谱'),
    );
    expect(start.onPressed, isNull);
    expect(stop.onPressed, isNotNull);
    expect(upload.onPressed, isNull);

    await tester.pumpWidget(const SizedBox.shrink());
    controller.dispose();
    await tester.binding.setSurfaceSize(null);
  });

  testWidgets('CSV export disables repeat taps and reports its result', (
    tester,
  ) async {
    tester.view.devicePixelRatio = 1;
    tester.view.physicalSize = const Size(1200, 900);
    addTearDown(() {
      tester.view.resetDevicePixelRatio();
      tester.view.resetPhysicalSize();
    });
    final controller = BleController(
      initializeBle: false,
      commandExecutor: _successResponse,
    );
    controller.records[SensorType.proximity]!.add(
      ProximityRecord(
        packetTimestamp: 1,
        recordIndex: 0,
        raw: Uint8List(4),
        channelA: 35,
        channelB: 121,
      ),
    );
    final exportCompleter = Completer<CsvExportResult>();
    await tester.pumpWidget(
      MaterialApp(
        home: HomePage(
          controller: controller,
          csvExporter: (_, _) => exportCompleter.future,
        ),
      ),
    );
    await tester.tap(find.text('接近'));
    await tester.pump();

    await tester.tap(find.widgetWithText(OutlinedButton, '导出 CSV'));
    await tester.pump();
    expect(
      tester
          .widget<OutlinedButton>(find.widgetWithText(OutlinedButton, '导出 CSV'))
          .onPressed,
      isNull,
    );

    exportCompleter.complete(
      const CsvExportResult(status: CsvExportStatus.shared, message: 'CSV分享完成'),
    );
    await tester.pumpAndSettle();
    expect(find.text('CSV分享完成'), findsOneWidget);

    await tester.pumpWidget(const SizedBox.shrink());
    controller.dispose();
  });

  testWidgets('desktop rail navigates across all five sensor pages', (
    tester,
  ) async {
    tester.view.devicePixelRatio = 1;
    tester.view.physicalSize = const Size(1200, 900);
    addTearDown(() {
      tester.view.resetDevicePixelRatio();
      tester.view.resetPhysicalSize();
    });
    final controller = BleController(
      initializeBle: false,
      commandExecutor: _successResponse,
    );
    await controller.startSensor(SensorType.spectrum);
    await controller.startSensor(SensorType.ultraviolet);
    await tester.pumpWidget(
      MaterialApp(home: HomePage(controller: controller)),
    );

    expect(find.text('启动剩余3类'), findsOneWidget);
    expect(find.text('页面导航'), findsOneWidget);
    expect(find.byType(Drawer), findsNothing);
    for (final sensor in SensorType.values) {
      await tester.tap(find.text(sensor.label));
      await tester.pump();
      expect(find.text('${sensor.label}采集与上传'), findsOneWidget);
      expect(find.text('单次采集${sensor.label}'), findsOneWidget);
      expect(find.text('上传${sensor.label}数据'), findsOneWidget);
      if (sensor == SensorType.spectrum || sensor == SensorType.ultraviolet) {
        expect(
          tester
              .widget<DropdownButton<int>>(find.byType(DropdownButton<int>))
              .onChanged,
          isNull,
        );
      }
    }

    await tester.pumpWidget(const SizedBox.shrink());
    controller.dispose();
  });

  for (final entry in <(SensorType, ParsedRecord, String, String)>[
    (
      SensorType.spectrum,
      SpectrumRecord(
        packetTimestamp: 0,
        recordIndex: 0,
        raw: Uint8List(22),
        gainCode: 10,
        channels: List<int>.filled(10, 10),
      ),
      '波长/通道（nm）',
      '原始计数（count）',
    ),
    (
      SensorType.flicker,
      FlickerRecord(
        packetTimestamp: 0,
        recordIndex: 0,
        raw: Uint8List(256),
        samples: List<int>.generate(128, (i) => i),
        validSampleEstimate: 128,
        dominantFrequencyHz: 100,
      ),
      '单次FIFO内相对时间（ms）',
      'Flicker ADC原始计数（count）',
    ),
    (
      SensorType.ultraviolet,
      UvRecord(
        packetTimestamp: 0,
        recordIndex: 0,
        raw: Uint8List(8),
        uva: 1,
        uvb: 2,
        uvc: 3,
        temperatureC: 25,
      ),
      '相对时间（min）',
      '辐照度（µW/cm²）',
    ),
    (
      SensorType.proximity,
      ProximityRecord(
        packetTimestamp: 0,
        recordIndex: 0,
        raw: Uint8List(4),
        channelA: 1,
        channelB: 2,
      ),
      '相对时间（min）',
      '原始计数（count）',
    ),
    (
      SensorType.acceleration,
      AccelerationRecord(
        packetTimestamp: 0,
        recordIndex: 0,
        raw: Uint8List(7),
        highSpeed: false,
        xRaw: 1,
        yRaw: 2,
        zRaw: 3,
        estimatedTimestamp: 0,
      ),
      '相对时间（s）',
      '加速度（g）',
    ),
  ]) {
    testWidgets('${entry.$1.label} chart shows axis units', (tester) async {
      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: SizedBox(
              width: 800,
              height: 360,
              child: SensorChart(sensor: entry.$1, records: [entry.$2]),
            ),
          ),
        ),
      );
      await tester.pump();
      expect(find.text(entry.$3), findsOneWidget);
      expect(find.text(entry.$4), findsOneWidget);
    });
  }
}
