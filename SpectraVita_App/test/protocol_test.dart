import 'dart:math' as math;
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:spectravita_app/protocol/spectravita_protocol.dart';

void main() {
  group('protocol frame', () {
    test('encodes documented upload command', () {
      expect(
        ProtocolFrame.toHex(DeviceCommands.upload(SensorType.acceleration)),
        'AA 50 05 55 BB',
      );
    });

    test('builds 15-byte periodic command with little-endian fields', () {
      final bytes = DeviceCommands.startPeriodic(
        SensorType.spectrum,
        unixSeconds: 0x12345678,
      );
      expect(bytes.length, 15);
      expect(bytes.sublist(0, 13), [
        0xaa,
        0x4e,
        0x78,
        0x56,
        0x34,
        0x12,
        0x3c,
        0x00,
        1,
        0,
        0,
        0,
        0,
      ]);
      expect(ProtocolFrame.decode(bytes).command, 0x4e);
    });

    test('rejects bad checksum', () {
      expect(
        () => ProtocolFrame.decode([0xaa, 0x53, 0x00, 0xbb]),
        throwsA(isA<ProtocolException>()),
      );
    });

    test('encodes all documented single capture commands', () {
      expect(
        ProtocolFrame.toHex(DeviceCommands.captureOnce(SensorType.spectrum)),
        'AA 42 1D 57 02 0A 00 BB',
      );
      expect(
        ProtocolFrame.toHex(DeviceCommands.captureOnce(SensorType.flicker)),
        'AA 44 D0 02 05 93 BB',
      );
      expect(
        ProtocolFrame.toHex(DeviceCommands.captureOnce(SensorType.ultraviolet)),
        'AA 46 09 4F BB',
      );
      expect(
        ProtocolFrame.toHex(DeviceCommands.captureOnce(SensorType.proximity)),
        'AA 48 03 4B BB',
      );
      expect(
        ProtocolFrame.toHex(
          DeviceCommands.captureOnce(SensorType.acceleration),
        ),
        'AA 4A 4A BB',
      );
      expect(SensorType.values.map(DeviceCommands.singleResponseCommand), [
        0x43,
        0x45,
        0x47,
        0x49,
        0x4b,
      ]);
    });

    test('encodes selectable spectrum and ultraviolet gain ranges', () {
      expect(
        ProtocolFrame.toHex(
          DeviceCommands.captureOnce(SensorType.spectrum, spectrumGainCode: 0),
        ),
        'AA 42 1D 57 02 00 0A BB',
      );
      expect(
        ProtocolFrame.toHex(
          DeviceCommands.captureOnce(
            SensorType.ultraviolet,
            ultravioletGainCode: 11,
          ),
        ),
        'AA 46 0B 4D BB',
      );
      expect(
        () => DeviceCommands.captureOnce(
          SensorType.spectrum,
          spectrumGainCode: 11,
        ),
        throwsA(isA<ProtocolException>()),
      );
      expect(
        () => DeviceCommands.captureOnce(
          SensorType.ultraviolet,
          ultravioletGainCode: 12,
        ),
        throwsA(isA<ProtocolException>()),
      );
    });

    test('encodes all selectable multispectral FIFO gain values', () {
      expect(
        ProtocolFrame.toHex(
          DeviceCommands.captureOnce(SensorType.flicker, flickerGainCode: 0),
        ),
        'AA 44 D0 02 00 96 BB',
      );
      expect(
        ProtocolFrame.toHex(
          DeviceCommands.captureOnce(SensorType.flicker, flickerGainCode: 10),
        ),
        'AA 44 D0 02 0A 9C BB',
      );
      expect(
        () =>
            DeviceCommands.captureOnce(SensorType.flicker, flickerGainCode: 11),
        throwsA(isA<ProtocolException>()),
      );
    });
  });

  group('sensor data parser', () {
    test('parses all single capture response formats', () {
      final spectrum = ByteData(20);
      for (var i = 0; i < 10; i++) {
        spectrum.setUint16(i * 2, 10 + i, Endian.little);
      }
      final spectrumResult = SensorDataParser.parseSingle(
        SensorType.spectrum,
        spectrum.buffer.asUint8List(),
        spectrumGainCode: 0,
      ) as SpectrumSingleMeasurement;
      expect(spectrumResult.gainCode, 0);
      expect(spectrumResult.displayValues['请求增益'], '0.5×');
      expect(spectrumResult.channels.last, 19);

      final flicker = ByteData(256);
      for (var i = 0; i < 128; i++) {
        flicker.setUint16(i * 2, i + 1, Endian.little);
      }
      final flickerResult = SensorDataParser.parseSingle(
        SensorType.flicker,
        flicker.buffer.asUint8List(),
        flickerGainCode: 10,
      ) as FlickerSingleMeasurement;
      expect(flickerResult.samples.length, 128);
      expect(flickerResult.validSampleEstimate, 128);
      expect(flickerResult.gainCode, 10);
      expect(flickerResult.displayValues['请求增益'], '512×');

      final uv = ByteData(8)
        ..setUint16(0, 1, Endian.little)
        ..setUint16(2, 2, Endian.little)
        ..setUint16(4, 3, Endian.little)
        ..setInt16(6, -25, Endian.little);
      final uvResult = SensorDataParser.parseSingle(
        SensorType.ultraviolet,
        uv.buffer.asUint8List(),
        ultravioletGainCode: 11,
      ) as UvSingleMeasurement;
      expect([uvResult.uva, uvResult.uvb, uvResult.uvc], [1, 2, 3]);
      expect(uvResult.temperatureC, -2.5);
      expect(uvResult.gainCode, 11);
      expect(uvResult.displayValues['请求增益'], '2048×');

      final proximity = ByteData(4)
        ..setUint16(0, 35, Endian.little)
        ..setUint16(2, 121, Endian.little);
      final proximityResult = SensorDataParser.parseSingle(
        SensorType.proximity,
        proximity.buffer.asUint8List(),
        proximityGainCode: 3,
      ) as ProximitySingleMeasurement;
      expect(proximityResult.channelA, 35);
      expect(proximityResult.channelB, 121);
      expect(proximityResult.gainCode, 3);

      final acceleration = ByteData(6)
        ..setInt16(0, -10, Endian.little)
        ..setInt16(2, 20, Endian.little)
        ..setInt16(4, -30, Endian.little);
      final accelerationResult = SensorDataParser.parseSingle(
        SensorType.acceleration,
        acceleration.buffer.asUint8List(),
      ) as AccelerationSingleMeasurement;
      expect(
        [
          accelerationResult.xRaw,
          accelerationResult.yRaw,
          accelerationResult.zRaw,
        ],
        [-10, 20, -30],
      );
      expect(accelerationResult.yG, closeTo(0.078, 1e-9));
    });

    test('rejects an incorrect single response length', () {
      expect(
        () => SensorDataParser.parseSingle(SensorType.proximity, Uint8List(3)),
        throwsA(isA<ProtocolException>()),
      );
      expect(
        () => SensorDataParser.parseSingle(SensorType.spectrum, Uint8List(22)),
        throwsA(isA<ProtocolException>()),
      );
    });
    test('parses spectrum gain and ten channels', () {
      final record = ByteData(22)..setUint16(0, 10, Endian.little);
      for (var i = 0; i < 10; i++) {
        record.setUint16(2 + i * 2, 100 + i, Endian.little);
      }
      final packet = SensorDataParser.parseUpload(
        SensorType.spectrum,
        _packetData(1000, record.buffer.asUint8List()),
      );
      final parsed = packet.records.single as SpectrumRecord;
      expect(parsed.gainCode, 10);
      expect(parsed.gainMultiplier, 512);
      expect(parsed.channels, List.generate(10, (i) => 100 + i));
    });

    test('parses UV order and signed temperature', () {
      final record = ByteData(8)
        ..setUint16(0, 11, Endian.little)
        ..setUint16(2, 22, Endian.little)
        ..setUint16(4, 33, Endian.little)
        ..setInt16(6, -55, Endian.little);
      final parsed =
          SensorDataParser.parseUpload(
                SensorType.ultraviolet,
                _packetData(2000, record.buffer.asUint8List()),
              ).records.single
              as UvRecord;
      expect([parsed.uva, parsed.uvb, parsed.uvc], [11, 22, 33]);
      expect(parsed.temperatureC, -5.5);
    });

    test('parses proximity little-endian raw channels', () {
      final record = ByteData(4)
        ..setUint16(0, 0x1234, Endian.little)
        ..setUint16(2, 800, Endian.little);
      final parsed =
          SensorDataParser.parseUpload(
                SensorType.proximity,
                _packetData(3000, record.buffer.asUint8List()),
              ).records.single
              as ProximityRecord;
      expect(parsed.channelA, 0x1234);
      expect(parsed.channelB, 800);
    });

    test('parses low/high acceleration and estimates timestamps', () {
      final low = ByteData(7)
        ..setUint8(0, 0)
        ..setInt16(1, 100, Endian.little)
        ..setInt16(3, -100, Endian.little)
        ..setInt16(5, 256, Endian.little);
      final high = ByteData(7)
        ..setUint8(0, 1)
        ..setInt16(1, -1, Endian.little)
        ..setInt16(3, 2, Endian.little)
        ..setInt16(5, -3, Endian.little);
      final parsed = SensorDataParser.parseUpload(
        SensorType.acceleration,
        _packetData(4000, [
          ...low.buffer.asUint8List(),
          ...high.buffer.asUint8List(),
        ]),
      ).records.cast<AccelerationRecord>();
      expect(parsed[0].highSpeed, isFalse);
      expect(parsed[0].xG, closeTo(0.39, 1e-9));
      expect(parsed[1].highSpeed, isTrue);
      expect(parsed[1].estimatedTimestamp, 4001);
      expect(parsed[1].zRaw, -3);
    });

    test('parses flicker samples and finds a dominant frequency', () {
      final record = ByteData(256);
      for (var i = 0; i < 128; i++) {
        record.setUint16(i * 2, i.isEven ? 1000 : 2000, Endian.little);
      }
      final parsed =
          SensorDataParser.parseUpload(
                SensorType.flicker,
                _packetData(5000, record.buffer.asUint8List()),
              ).records.single
              as FlickerRecord;
      expect(parsed.samples.length, 128);
      expect(parsed.validSampleEstimate, 128);
      expect(parsed.dominantFrequencyHz, isNotNull);
    });

    test('suppresses a frequency result when FIFO signal is too weak', () {
      final record = ByteData(256);
      for (var i = 0; i < 128; i++) {
        record.setUint16(i * 2, i.isEven ? 0 : 1, Endian.little);
      }
      final parsed =
          SensorDataParser.parseUpload(
                SensorType.flicker,
                _packetData(5000, record.buffer.asUint8List()),
              ).records.single
              as FlickerRecord;
      expect(parsed.signalRange, 1);
      expect(parsed.signalTooWeak, isTrue);
      expect(parsed.dominantFrequencyHz, isNull);
      expect(parsed.tableValues[1], contains('信号过弱'));
    });

    for (final targetHz in [100.0, 120.0]) {
      test('detects a strong ${targetHz.toInt()} Hz FIFO waveform', () {
        final record = ByteData(256);
        final sampleRate = 1 / FlickerRecord.sampleIntervalSeconds;
        for (var i = 0; i < 128; i++) {
          final value =
              1000 +
              (200 * math.sin(2 * math.pi * targetHz * i / sampleRate)).round();
          record.setUint16(i * 2, value, Endian.little);
        }
        final parsed =
            SensorDataParser.parseUpload(
                  SensorType.flicker,
                  _packetData(5000, record.buffer.asUint8List()),
                ).records.single
                as FlickerRecord;
        expect(parsed.signalTooWeak, isFalse);
        expect(parsed.dominantFrequencyHz, closeTo(targetHz, 2.1));
      });
    }

    test('uses FD_TIME times 2.78 microseconds for FIFO samples', () {
      expect(FlickerRecord.sampleIntervalSeconds, closeTo(0.0020016, 1e-12));
      expect(1 / FlickerRecord.sampleIntervalSeconds, closeTo(499.6003, 1e-4));
    });

    test('rejects misaligned record payload', () {
      expect(
        () => SensorDataParser.parseUpload(
          SensorType.acceleration,
          Uint8List.fromList([0, 0, 0, 0, 1, 2]),
        ),
        throwsA(isA<ProtocolException>()),
      );
    });
  });
}

Uint8List _packetData(int timestamp, List<int> records) {
  final result = Uint8List(4 + records.length);
  ByteData.sublistView(result).setUint32(0, timestamp, Endian.little);
  result.setRange(4, result.length, records);
  return result;
}
