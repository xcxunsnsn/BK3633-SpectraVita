import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:intl/intl.dart';
import 'package:share_plus/share_plus.dart' as share;
import 'package:spectravita_app/protocol/spectravita_protocol.dart';
import 'package:spectravita_app/services/data_exporter.dart';

void main() {
  final records = <SensorType, ParsedRecord>{
    SensorType.spectrum: SpectrumRecord(
      packetTimestamp: 1,
      recordIndex: 0,
      raw: Uint8List(22),
      gainCode: 10,
      channels: List<int>.filled(10, 5),
    ),
    SensorType.flicker: FlickerRecord(
      packetTimestamp: 1,
      recordIndex: 0,
      raw: Uint8List(256),
      samples: List<int>.filled(128, 2),
      validSampleEstimate: 128,
      dominantFrequencyHz: 100,
    ),
    SensorType.ultraviolet: UvRecord(
      packetTimestamp: 1,
      recordIndex: 0,
      raw: Uint8List(8),
      uva: 1,
      uvb: 2,
      uvc: 3,
      temperatureC: 25,
    ),
    SensorType.proximity: ProximityRecord(
      packetTimestamp: 1,
      recordIndex: 0,
      raw: Uint8List(4),
      channelA: 35,
      channelB: 121,
    ),
    SensorType.acceleration: AccelerationRecord(
      packetTimestamp: 1,
      recordIndex: 0,
      raw: Uint8List(7),
      highSpeed: false,
      xRaw: 1,
      yRaw: 2,
      zRaw: 3,
      estimatedTimestamp: 1,
    ),
  };

  test('all five CSV types contain UTF-8 BOM and unit headers', () {
    const expectedHeaders = <SensorType, String>{
      SensorType.spectrum: '415_raw_count',
      SensorType.flicker: 'sample_0_raw_count',
      SensorType.ultraviolet: 'UVA_uW_cm2',
      SensorType.proximity: 'A_raw_count_1x',
      SensorType.acceleration: 'X_m_s2',
    };

    for (final sensor in SensorType.values) {
      final bytes = DataExporter.buildCsvBytes(sensor, [records[sensor]!]);
      expect(bytes.take(3), [0xef, 0xbb, 0xbf]);
      final text = utf8.decode(bytes);
      expect(text, contains(expectedHeaders[sensor]));
      expect(text, contains('记录时间_unix_s_估算'));
      expect(text, contains('记录时间_本地_估算'));
    }
  });

  test('CSV file name is stable and safe for Windows and Android', () {
    expect(
      DataExporter.buildFileName(
        SensorType.proximity,
        now: DateTime.utc(2026, 9, 10, 12, 34, 56),
      ),
      'SpectraVita_proximity_2026-09-10T12-34-56.000Z.csv',
    );
    expect(
      DataExporter.buildFileName(
        SensorType.flicker,
        now: DateTime.utc(2026, 9, 10, 12, 34, 56),
      ),
      'SpectraVita_multispectral_fifo_2026-09-10T12-34-56.000Z.csv',
    );
  });

  test('estimates readable per-record local timestamps', () {
    final spectrum = SpectrumRecord(
      packetTimestamp: 1000,
      recordIndex: 2,
      raw: Uint8List(22),
      gainCode: 10,
      channels: List<int>.filled(10, 5),
    );
    final highAcceleration = AccelerationRecord(
      packetTimestamp: 4000,
      recordIndex: 1,
      raw: Uint8List(7),
      highSpeed: true,
      xRaw: 1,
      yRaw: 2,
      zRaw: 3,
      estimatedTimestamp: 4000.125,
    );

    expect(DataExporter.estimatedRecordUnixSeconds(spectrum), 1120);
    expect(
      DataExporter.localRecordTime(spectrum),
      DateFormat('yyyy-MM-dd HH:mm:ss').format(
        DateTime.fromMillisecondsSinceEpoch(1120000, isUtc: true).toLocal(),
      ),
    );
    expect(
      DataExporter.estimatedRecordUnixSeconds(highAcceleration),
      '4000.125',
    );
    expect(
      DataExporter.localRecordTime(highAcceleration),
      DateFormat('yyyy-MM-dd HH:mm:ss.SSS').format(
        DateTime.fromMillisecondsSinceEpoch(4000125, isUtc: true).toLocal(),
      ),
    );
  });

  group('Android CSV sharing', () {
    late Directory directory;

    setUp(() async {
      directory = await Directory.systemTemp.createTemp('spectravita_csv_');
    });

    tearDown(() async {
      if (await directory.exists()) await directory.delete(recursive: true);
    });

    for (final entry in <share.ShareResultStatus, CsvExportStatus>{
      share.ShareResultStatus.success: CsvExportStatus.shared,
      share.ShareResultStatus.dismissed: CsvExportStatus.cancelled,
      share.ShareResultStatus.unavailable: CsvExportStatus.unavailable,
    }.entries) {
      test('maps ${entry.key.name} result and creates a CSV file', () async {
        share.ShareParams? received;
        final result = await DataExporter.exportCsv(
          SensorType.proximity,
          [records[SensorType.proximity]!],
          androidOverride: true,
          temporaryDirectoryOverride: directory,
          now: DateTime.utc(2026, 9, 10),
          shareInvoker: (params) async {
            received = params;
            return share.ShareResult('test', entry.key);
          },
        );

        expect(result.status, entry.value);
        expect(received?.files, hasLength(1));
        expect(received?.files?.single.mimeType, 'text/csv');
        expect(await File(received!.files!.single.path).exists(), isTrue);
        expect(received?.fileNameOverrides?.single, endsWith('.csv'));
      });
    }

    test('returns an explicit failure instead of throwing silently', () async {
      final result = await DataExporter.exportCsv(
        SensorType.proximity,
        [records[SensorType.proximity]!],
        androidOverride: true,
        temporaryDirectoryOverride: directory,
        shareInvoker: (_) async => throw StateError('share unavailable'),
      );

      expect(result.status, CsvExportStatus.failed);
      expect(result.message, contains('CSV导出失败'));
      expect(result.message, contains('share unavailable'));
    });
  });
}
