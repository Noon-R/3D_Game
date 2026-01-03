# DirectX 12 3D プラットフォーマー

DirectX 12を使用したシンプルな3Dプラットフォーマーゲームの雛形です。

## 概要

このプロジェクトは、DirectX 12を使用した基本的な3Dプラットフォーマーゲームの実装です。プレイヤーは3D空間内のプラットフォーム上を移動し、ジャンプすることができます。

## 主な機能

- **DirectX 12レンダリング**: モダンなグラフィックスAPI
- **3Dグラフィックス**: 立方体ベースのプレイヤーとプラットフォーム
- **物理演算**: 重力、ジャンプ、衝突判定
- **キーボード入力**: WASD移動、スペースキーでジャンプ
- **カメラシステム**: 固定視点カメラ

## 必要要件

- Windows 10/11
- Visual Studio 2019以降
- Windows SDK 10.0.19041.0以降
- DirectX 12対応GPU

## プロジェクト構造

```
3D_Game/
├── include/          # ヘッダーファイル
│   ├── DX12Core.h   # DirectX 12コアクラス
│   ├── Player.h     # プレイヤークラス
│   ├── Platform.h   # プラットフォームクラス
│   └── d3dx12.h     # DirectX 12ヘルパー
├── src/             # ソースファイル
│   ├── DX12Core.cpp
│   ├── Player.cpp
│   ├── Platform.cpp
│   └── main.cpp     # エントリポイント
└── shaders/         # HLSLシェーダー
    ├── VertexShader.hlsl
    └── PixelShader.hlsl
```

## ビルド手順

### Visual Studioを使用する場合

1. Visual Studio 2019以降を開く
2. 「新しいプロジェクトの作成」→「空のプロジェクト」を選択
3. プロジェクト名を「3D_Game」にして作成
4. ソリューションエクスプローラーで以下を追加：
   - `include/`フォルダ内の全ファイルをヘッダーファイルとして追加
   - `src/`フォルダ内の全ファイルをソースファイルとして追加

5. プロジェクトのプロパティを設定：
   - 構成プロパティ → 全般
     - Windows SDK バージョン: 10.0.19041.0以降
   - 構成プロパティ → C/C++ → 全般
     - 追加のインクルードディレクトリ: `$(ProjectDir)include`
   - 構成プロパティ → リンカー → システム
     - サブシステム: Windows (/SUBSYSTEM:WINDOWS)
   - 構成プロパティ → リンカー → 入力
     - 追加の依存ファイル: `d3d12.lib;dxgi.lib;d3dcompiler.lib`

6. シェーダーファイルの設定：
   - `shaders/VertexShader.hlsl`を右クリック → プロパティ
     - 項目の種類: カスタムビルドツール → HLSL コンパイラ
   - `shaders/PixelShader.hlsl`も同様に設定

7. ビルド（F7）して実行（F5）

### 手動ビルド（コマンドライン）

```batch
# Visual Studio Developer Command Promptを開く

# コンパイル
cl /EHsc /I"include" /D_UNICODE /DUNICODE src\main.cpp src\DX12Core.cpp src\Player.cpp src\Platform.cpp ^
   /link d3d12.lib dxgi.lib d3dcompiler.lib user32.lib gdi32.lib /SUBSYSTEM:WINDOWS /OUT:3DGame.exe

# シェーダーのコンパイル（実行時にコンパイルされるため省略可能）
```

## 操作方法

- **W/A/S/D**: 前後左右移動
- **スペースキー**: ジャンプ
- **ESC**: 終了

## アーキテクチャ

### DX12Core
DirectX 12の初期化と管理を担当：
- デバイスとコマンドキューの作成
- スワップチェーンの管理
- デスクリプタヒープの管理
- パイプラインステートの作成
- レンダリングコマンドの記録と実行

### Player
プレイヤーの状態と動作を管理：
- 位置、速度の管理
- 移動とジャンプの処理
- ワールド行列の計算

### Platform
プラットフォームオブジェクトを表現：
- 位置とサイズの管理
- 衝突判定
- ワールド行列の計算

## 拡張のアイデア

- テクスチャマッピング
- ライティング（法線マップ、スペキュラ）
- より複雑なレベルデザイン
- パーティクルエフェクト
- サウンド
- UI要素（スコア、ライフなど）
- セーブ/ロード機能

## トラブルシューティング

### DirectX 12が初期化できない
- GPUがDirectX 12に対応しているか確認
- グラフィックドライバーを最新に更新
- Windows 10/11を使用しているか確認

### シェーダーのコンパイルエラー
- `shaders/`フォルダが実行ファイルと同じディレクトリにあるか確認
- シェーダーファイルのパスが正しいか確認

### リンクエラー
- Windows SDKが正しくインストールされているか確認
- プロジェクト設定でライブラリ（d3d12.lib, dxgi.lib, d3dcompiler.lib）が追加されているか確認

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。

## 参考資料

- [DirectX 12 プログラミングガイド](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [DirectX-Graphics-Samples](https://github.com/microsoft/DirectX-Graphics-Samples)
