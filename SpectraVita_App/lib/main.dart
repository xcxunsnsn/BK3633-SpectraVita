import 'package:flutter/material.dart';

import 'screens/home_page.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const SpectraVitaApp());
}

class SpectraVitaApp extends StatelessWidget {
  const SpectraVitaApp({super.key});

  @override
  Widget build(BuildContext context) {
    const seed = Color(0xff216869);
    return MaterialApp(
      title: 'SpectraVita 传感器工具',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        fontFamily: 'SourceHanSansCN',
        colorScheme: ColorScheme.fromSeed(
          seedColor: seed,
          brightness: Brightness.light,
        ),
        scaffoldBackgroundColor: const Color(0xfff4f7f6),
        cardTheme: const CardThemeData(
          elevation: 0,
          margin: EdgeInsets.zero,
          clipBehavior: Clip.antiAlias,
        ),
        inputDecorationTheme: const InputDecorationTheme(
          border: OutlineInputBorder(),
        ),
      ),
      home: const HomePage(),
    );
  }
}
