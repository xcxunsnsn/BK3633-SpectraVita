import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:csv/csv.dart';
import 'package:file_selector/file_selector.dart' as file_selector;
import 'package:intl/intl.dart';
import 'package:path_provider/path_provider.dart';
import 'package:share_plus/share_plus.dart' as share;

import '../protocol/spectravita_protocol.dart';

enum CsvExportStatus { saved, shared, cancelled, unavailable, failed }

class CsvExportResult {
  const CsvExportResult({
    required this.status,
    required this.message,
    this.path,
  });

  final CsvExportStatus status;
  final String message;
  final String? path;
}

typedef CsvShareInvoker = Future<share.ShareResult> Function(
  share.ShareParams params,
);

class DataExporter {
  const DataExporter._();

  static Future<CsvExportResult> exportCsv(
    SensorType sensor,
    List<ParsedRecord> records, {
    bool? androidOverride,
    Directory? temporaryDirectoryOverride,
    CsvShareInvoker? shareInvoker,
    DateTime? now,
  }) async {
    try {
      final bytes = buildCsvBytes(sensor, records);
      final fileName = buildFileName(sensor, now: now);
      if (androidOverride ?? Platform.isAndroid) {
        return await _shareAndroidCsv(
          bytes,
          fileName,
          temporaryDirectory: temporaryDirectoryOverride,
          shareInvoker: shareInvoker,
        );
      }
      return await _saveDesktopCsv(bytes, fileName);
    } catch (error) {
      return CsvExportResult(
        status: CsvExportStatus.failed,
        message: 'CSV导出失败：$error',
      );
    }
  }

  static Uint8List buildCsvBytes(
    SensorType sensor,
    List<ParsedRecord> records,
  ) {
    final rows = <List<Object?>>[
      _headers(sensor),
      for (final record in records)
        [
          record.packetTimestamp,
          record.recordIndex,
          estimatedRecordUnixSeconds(record),
          localRecordTime(record),
          ProtocolFrame.toHex(record.raw),
          ...record.csvValues,
        ],
    ];
    final csvText = Csv(addBom: true).encode(rows);
    return Uint8List.fromList(utf8.encode(csvText));
  }

  static String buildFileName(SensorType sensor, {DateTime? now}) {
    final timestamp = (now ?? DateTime.now()).toIso8601String().replaceAll(
      ':',
      '-',
    );
    final sensorName = sensor == SensorType.flicker
        ? 'multispectral_fifo'
        : sensor.name;
    return 'SpectraVita_${sensorName}_$timestamp.csv';
  }

  static Object estimatedRecordUnixSeconds(ParsedRecord record) {
    final seconds = record is AccelerationRecord
        ? record.estimatedTimestamp
        : record.packetTimestamp +
              record.recordIndex * record.sensor.periodSeconds;
    return record is AccelerationRecord
        ? seconds.toStringAsFixed(3)
        : seconds.round();
  }

  static String localRecordTime(ParsedRecord record) {
    final seconds = record is AccelerationRecord
        ? record.estimatedTimestamp
        : record.packetTimestamp +
              record.recordIndex * record.sensor.periodSeconds;
    final time = DateTime.fromMillisecondsSinceEpoch(
      (seconds * 1000).round(),
      isUtc: true,
    ).toLocal();
    final pattern = record is AccelerationRecord && record.highSpeed
        ? 'yyyy-MM-dd HH:mm:ss.SSS'
        : 'yyyy-MM-dd HH:mm:ss';
    return DateFormat(pattern).format(time);
  }

  static Future<CsvExportResult> _saveDesktopCsv(
    Uint8List bytes,
    String fileName,
  ) async {
    final location = await file_selector.getSaveLocation(
      suggestedName: fileName,
      acceptedTypeGroups: const [
        file_selector.XTypeGroup(label: 'CSV', extensions: ['csv']),
      ],
    );
    if (location == null) {
      return const CsvExportResult(
        status: CsvExportStatus.cancelled,
        message: '已取消导出',
      );
    }
    await file_selector.XFile.fromData(
      bytes,
      mimeType: 'text/csv',
      name: fileName,
    ).saveTo(location.path);
    return CsvExportResult(
      status: CsvExportStatus.saved,
      message: 'CSV已保存：${location.path}',
      path: location.path,
    );
  }

  static Future<CsvExportResult> _shareAndroidCsv(
    Uint8List bytes,
    String fileName, {
    Directory? temporaryDirectory,
    CsvShareInvoker? shareInvoker,
  }) async {
    final directory = temporaryDirectory ?? await getTemporaryDirectory();
    final file = File('${directory.path}${Platform.pathSeparator}$fileName');
    await file.writeAsBytes(bytes, flush: true);
    final params = share.ShareParams(
      title: '导出传感器CSV',
      subject: fileName,
      text: 'SpectraVita传感器数据',
      files: [share.XFile(file.path, mimeType: 'text/csv', name: fileName)],
      fileNameOverrides: [fileName],
    );
    final result = await (shareInvoker ?? share.SharePlus.instance.share)(
      params,
    );
    switch (result.status) {
      case share.ShareResultStatus.success:
        return CsvExportResult(
          status: CsvExportStatus.shared,
          message: 'CSV分享完成',
          path: file.path,
        );
      case share.ShareResultStatus.dismissed:
        return CsvExportResult(
          status: CsvExportStatus.cancelled,
          message: '用户取消导出',
          path: file.path,
        );
      case share.ShareResultStatus.unavailable:
        return CsvExportResult(
          status: CsvExportStatus.unavailable,
          message: '没有可用的分享应用',
          path: file.path,
        );
    }
  }

  static List<Object?> _headers(SensorType sensor) {
    const prefix = <Object?>[
      '包时间戳_unix_s',
      '包内序号',
      '记录时间_unix_s_估算',
      '记录时间_本地_估算',
      '原始HEX',
    ];
    switch (sensor) {
      case SensorType.spectrum:
        return [
          ...prefix,
          '增益代码',
          '增益倍数',
          ...SpectrumRecord.wavelengths.map((e) => '${e}_raw_count'),
          ...SpectrumRecord.wavelengths.map(
            (e) => '${e}_gain_normalized_count',
          ),
        ];
      case SensorType.flicker:
        return [
          ...prefix,
          '估算有效点数',
          '主频_Hz',
          ...List.generate(128, (i) => 'sample_${i}_raw_count'),
        ];
      case SensorType.ultraviolet:
        return [...prefix, 'UVA_uW_cm2', 'UVB_uW_cm2', 'UVC_uW_cm2', '温度_C'];
      case SensorType.proximity:
        return [...prefix, 'A_raw_count_1x', 'B_raw_count_command_gain'];
      case SensorType.acceleration:
        return [
          ...prefix,
          '模式',
          'X_raw_LSB',
          'Y_raw_LSB',
          'Z_raw_LSB',
          'X_g',
          'Y_g',
          'Z_g',
          'X_m_s2',
          'Y_m_s2',
          'Z_m_s2',
          '估算时间戳_unix_s',
        ];
    }
  }
}
