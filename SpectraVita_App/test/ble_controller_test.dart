import 'dart:async';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:spectravita_app/protocol/spectravita_protocol.dart';
import 'package:spectravita_app/services/ble_controller.dart';

void main() {
  BleCommandExecutor successfulExecutor(List<ProtocolFrame> sent) {
    return (
      Uint8List command, {
      required int expectedResponse,
      required Duration timeout,
    }) async {
      final request = ProtocolFrame.decode(command);
      sent.add(request);
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
      throw StateError('Unexpected response 0x${hexByte(expectedResponse)}');
    };
  }

  test('start all prepares gains then starts all five schedules', () async {
    final sent = <ProtocolFrame>[];
    final controller = BleController(
      initializeBle: false,
      commandExecutor: successfulExecutor(sent),
    );

    await controller.startAllSensors();

    expect(sent.map((frame) => frame.command), [
      0x42,
      0x4e,
      0x44,
      0x4e,
      0x46,
      0x4e,
      0x48,
      0x4e,
      0x4e,
    ]);
    expect(controller.activeSchedules, containsAll(SensorType.values));
    final selections = sent
        .where((frame) => frame.command == 0x4e)
        .map((frame) => frame.data.sublist(6, 11).indexOf(1))
        .toList();
    expect(selections, [0, 1, 2, 3, 4]);
    controller.dispose();
  });

  test('start all only starts schedules that are still missing', () async {
    final sent = <ProtocolFrame>[];
    final controller = BleController(
      initializeBle: false,
      commandExecutor: successfulExecutor(sent),
    );
    await controller.startSensor(SensorType.spectrum);
    await controller.startSensor(SensorType.ultraviolet);
    sent.clear();

    await controller.startAllSensors();

    expect(sent.map((frame) => frame.command), [0x44, 0x4e, 0x48, 0x4e, 0x4e]);
    final selections = sent
        .where((frame) => frame.command == 0x4e)
        .map((frame) => frame.data.sublist(6, 11).indexOf(1))
        .toList();
    expect(selections, [1, 3, 4]);
    expect(controller.activeSchedules.length, 5);
    controller.dispose();
  });

  test('explicit schedule rejection preserves successful schedules', () async {
    var scheduleCount = 0;
    final controller = BleController(
      initializeBle: false,
      commandExecutor:
          (command, {required expectedResponse, required timeout}) async {
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
            scheduleCount++;
            return ProtocolFrame(
              command: 0x4f,
              data: Uint8List.fromList([scheduleCount == 3 ? 1 : 0]),
            );
          },
    );

    await expectLater(
      controller.startAllSensors(),
      throwsA(isA<ProtocolException>()),
    );

    expect(controller.activeSchedules, {
      SensorType.spectrum,
      SensorType.flicker,
    });
    expect(controller.collectionStateUnknown, isFalse);
    controller.dispose();
  });

  test(
    'selected spectrum, FIFO and UV gains are sent before their schedules',
    () async {
      final sent = <ProtocolFrame>[];
      final controller = BleController(
        initializeBle: false,
        commandExecutor: successfulExecutor(sent),
      );
      controller.setSpectrumGainCode(0);
      controller.setFlickerGainCode(10);
      controller.setUltravioletGainCode(11);

      await controller.startSensor(SensorType.spectrum);
      await controller.startSensor(SensorType.flicker);
      await controller.startSensor(SensorType.ultraviolet);

      expect(sent.map((frame) => frame.command), [
        0x42,
        0x4e,
        0x44,
        0x4e,
        0x46,
        0x4e,
      ]);
      expect(sent[0].data.last, 0);
      expect(sent[2].data.last, 10);
      expect(sent[4].data.single, 11);
      expect(
        (controller.singleResults[SensorType.spectrum]
                as SpectrumSingleMeasurement)
            .gainCode,
        0,
      );
      expect(
        (controller.singleResults[SensorType.flicker]
                as FlickerSingleMeasurement)
            .gainCode,
        10,
      );
      expect(
        (controller.singleResults[SensorType.ultraviolet]
                as UvSingleMeasurement)
            .gainCode,
        11,
      );
      controller.dispose();
    },
  );

  test(
    'proximity schedule preparation automatically lowers saturated gain',
    () async {
      final sent = <ProtocolFrame>[];
      final controller = BleController(
        initializeBle: false,
        commandExecutor:
            (command, {required expectedResponse, required timeout}) async {
              final request = ProtocolFrame.decode(command);
              sent.add(request);
              if (expectedResponse == 0x49) {
                final gainCode = request.data.single;
                final data = ByteData(4)
                  ..setUint16(0, 1200, Endian.little)
                  ..setUint16(
                    2,
                    gainCode == 0 ? 1500 : proximityMaxCount,
                    Endian.little,
                  );
                return ProtocolFrame(
                  command: 0x49,
                  data: data.buffer.asUint8List(),
                );
              }
              return ProtocolFrame(
                command: 0x4f,
                data: Uint8List.fromList([0]),
              );
            },
      );

      await controller.startSensor(SensorType.proximity);

      expect(sent.map((frame) => frame.command), [
        0x48,
        0x48,
        0x48,
        0x48,
        0x4e,
      ]);
      expect(
        sent
            .where((frame) => frame.command == 0x48)
            .map((frame) => frame.data.single),
        [3, 2, 1, 0],
      );
      expect(controller.proximityGainCode, 0);
      expect(controller.activeSchedules, contains(SensorType.proximity));
      controller.dispose();
    },
  );

  test('proximity at full scale on 1x prevents schedule start', () async {
    final sent = <ProtocolFrame>[];
    final controller = BleController(
      initializeBle: false,
      commandExecutor:
          (command, {required expectedResponse, required timeout}) async {
            final request = ProtocolFrame.decode(command);
            sent.add(request);
            final data = ByteData(4)
              ..setUint16(0, 1200, Endian.little)
              ..setUint16(2, proximityMaxCount, Endian.little);
            return ProtocolFrame(
              command: 0x49,
              data: data.buffer.asUint8List(),
            );
          },
    );
    controller.setProximityGainCode(0);

    await expectLater(
      controller.startSensor(SensorType.proximity),
      throwsA(isA<ProtocolException>()),
    );

    expect(sent.map((frame) => frame.command), [0x48]);
    expect(controller.activeSchedules, isEmpty);
    expect(controller.collectionStateUnknown, isFalse);
    controller.dispose();
  });

  test(
    'ordinary proximity capture reports saturation without changing gain',
    () async {
      final controller = BleController(
        initializeBle: false,
        commandExecutor:
            (command, {required expectedResponse, required timeout}) async {
              final data = ByteData(4)
                ..setUint16(0, 1200, Endian.little)
                ..setUint16(2, proximityMaxCount, Endian.little);
              return ProtocolFrame(
                command: 0x49,
                data: data.buffer.asUint8List(),
              );
            },
      );

      final result = await controller.captureOnce(
        SensorType.proximity,
      ) as ProximitySingleMeasurement;

      expect(result.channelBSaturated, isTrue);
      expect(result.displayValues.values.join(), contains('饱和'));
      expect(controller.proximityGainCode, 3);
      controller.dispose();
    },
  );

  test('gain preparation timeout never sends the schedule command', () async {
    final sent = <ProtocolFrame>[];
    final controller = BleController(
      initializeBle: false,
      commandExecutor:
          (command, {required expectedResponse, required timeout}) async {
            sent.add(ProtocolFrame.decode(command));
            throw TimeoutException('gain response timeout');
          },
    );

    await expectLater(
      controller.startSensor(SensorType.spectrum),
      throwsA(isA<TimeoutException>()),
    );

    expect(sent.map((frame) => frame.command), [0x42]);
    expect(controller.activeSchedules, isEmpty);
    expect(controller.collectionStateUnknown, isTrue);
    controller.dispose();
  });

  test('malformed gain response never starts the schedule', () async {
    final sent = <ProtocolFrame>[];
    final controller = BleController(
      initializeBle: false,
      commandExecutor:
          (command, {required expectedResponse, required timeout}) async {
            sent.add(ProtocolFrame.decode(command));
            return ProtocolFrame(command: 0x43, data: Uint8List(19));
          },
    );

    await expectLater(
      controller.startSensor(SensorType.spectrum),
      throwsA(isA<ProtocolException>()),
    );

    expect(sent.map((frame) => frame.command), [0x42]);
    expect(controller.activeSchedules, isEmpty);
    expect(controller.collectionStateUnknown, isFalse);
    controller.dispose();
  });

  test('stop all clears every confirmed active schedule', () async {
    final sent = <ProtocolFrame>[];
    final controller = BleController(
      initializeBle: false,
      commandExecutor: successfulExecutor(sent),
    );
    await controller.startSensor(SensorType.acceleration);

    await controller.stopAllSensors();

    expect(controller.activeSchedules, isEmpty);
    expect(sent.last.command, 0x52);
    controller.dispose();
  });

  test(
    'disconnect preserves last active schedules and marks state unknown',
    () async {
      final sent = <ProtocolFrame>[];
      final controller = BleController(
        initializeBle: false,
        commandExecutor: successfulExecutor(sent),
      );
      await controller.startSensor(SensorType.spectrum);

      await controller.disconnect();

      expect(controller.connected, isFalse);
      expect(controller.activeSchedules, {SensorType.spectrum});
      expect(controller.collectionStateUnknown, isTrue);
      expect(controller.status, contains('可能仍在继续'));
      controller.dispose();
    },
  );
}
