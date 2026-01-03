# DirectX 12 3Dプラットフォーマー - Claude実装ドキュメント

## プロジェクト概要

このプロジェクトは、DirectX 12を使用した3Dプラットフォーマーゲームの雛形です。Claudeによって設計・実装されました。

## 実装日時

- 実装日: 2026年1月3日
- ブランチ: `claude/directx-3d-platformer-eaqU9`

## アーキテクチャ設計

### 1. DirectX 12コアシステム (DX12Core)

DirectX 12は低レベルAPIであり、DirectX 11と比較して以下の点で大きく異なります：

#### 主な特徴

**コマンドベースのアーキテクチャ**
- コマンドアロケータ: GPU命令のメモリ管理
- コマンドリスト: 描画命令の記録
- コマンドキュー: GPU実行キューへの送信

**明示的なリソース管理**
- デスクリプタヒープ: リソースの記述子を一元管理
- リソースバリア: リソース状態の明示的な遷移
- ルートシグネチャ: シェーダーリソースのレイアウト定義

**同期制御**
- フェンス: CPU/GPU間の同期ポイント
- フレームごとのリソース管理: ダブルバッファリング

#### 初期化フロー

```
1. D3D12CreateDevice() - デバイス作成
2. CreateCommandQueue() - コマンドキュー作成
3. CreateSwapChain() - スワップチェーン作成（2フレームバッファ）
4. CreateDescriptorHeaps() - RTV/DSV/CBVヒープ作成
5. CreateRenderTargetViews() - レンダーターゲットビュー作成
6. CreateDepthStencil() - 深度バッファ作成
7. CreateRootSignature() - ルートシグネチャ作成
8. CreatePipelineState() - パイプラインステート作成
9. CreateCommandAllocators() - コマンドアロケータ作成（フレーム数分）
10. CreateCommandList() - コマンドリスト作成
11. CreateFence() - フェンス作成
12. CreateVertexBuffer() - 頂点バッファ作成
13. CreateIndexBuffer() - インデックスバッファ作成
14. CreateConstantBuffer() - 定数バッファ作成
```

#### レンダリングフロー

```
BeginFrame():
  1. コマンドアロケータをリセット
  2. コマンドリストをリセット
  3. ルートシグネチャをセット
  4. デスクリプタヒープをセット
  5. ビューポートとシザー矩形をセット
  6. リソースバリア（PRESENT → RENDER_TARGET）
  7. レンダーターゲットとデプスステンシルをセット
  8. クリア処理
  9. 頂点バッファとインデックスバッファをセット

RenderCube():
  1. 定数バッファを更新（World/View/Projection行列）
  2. DrawIndexedInstanced()で描画

EndFrame():
  1. リソースバリア（RENDER_TARGET → PRESENT）
  2. コマンドリストをクローズ
  3. コマンドキューに送信
  4. Present()で画面に表示
  5. MoveToNextFrame()で次フレームへ
```

### 2. ゲームロジック

#### Playerクラス

**物理演算**
- 重力: 20.0 units/s²
- ジャンプ力: 10.0 units/s
- 移動速度: 5.0 units/s
- 速度減衰: 0.9（摩擦シミュレーション）

**状態管理**
- 位置（Position）
- 速度（Velocity）
- 接地状態（IsGrounded）

#### Platformクラス

**衝突判定**
- AABB（Axis-Aligned Bounding Box）方式
- プレイヤーとプラットフォームの3D衝突チェック
- Y軸方向の衝突応答（プラットフォームの上に乗る）

### 3. シェーダーシステム

#### 頂点シェーダー（VertexShader.hlsl）

```hlsl
入力:
  - POSITION: float3（ローカル座標）
  - COLOR: float4（頂点カラー）

処理:
  1. World変換
  2. View変換
  3. Projection変換

出力:
  - SV_POSITION: float4（スクリーン座標）
  - COLOR: float4（補間用カラー）
```

#### ピクセルシェーダー（PixelShader.hlsl）

```hlsl
入力:
  - COLOR: float4（頂点から補間されたカラー）

出力:
  - SV_TARGET: float4（最終カラー）
```

### 4. 入力システム

**キーボード入力**
- グローバル配列でキー状態を管理
- WM_KEYDOWN/WM_KEYUPイベントでキー状態を更新
- ゲームループ内でポーリング

**マッピング**
- W: 前進
- S: 後退
- A: 左移動
- D: 右移動
- Space: ジャンプ
- ESC: 終了

## 技術的な決定事項

### DirectX 12を選択した理由

1. **モダンなAPI**: 最新のGPU機能にアクセス可能
2. **明示的な制御**: リソース管理と同期を細かく制御
3. **パフォーマンス**: マルチスレッド対応、低オーバーヘッド
4. **学習価値**: 現代のグラフィックスAPIの理解に最適

### リソース管理戦略

**ダブルバッファリング**
- フレーム数: 2
- 各フレームごとに独立したコマンドアロケータ
- フェンスによる同期で前フレームの完了を保証

**定数バッファの最適化**
- 256バイトアライメント: GPUの要件に準拠
- マップ/アンマップ: 初期化時に一度だけマップし、終了時にアンマップ
- 毎フレーム更新: memcpyで高速コピー

**リソース状態遷移**
- PRESENT ↔ RENDER_TARGET: フレームの開始/終了時
- DEPTH_WRITE: 深度バッファは常にこの状態

### 物理演算の実装

**シンプルな重力シミュレーション**
```cpp
velocity.y -= gravity * deltaTime;  // 重力加速度
position += velocity * deltaTime;    // オイラー積分
```

**衝突応答**
- プラットフォームの上面との衝突のみ検出
- Y方向の速度をゼロにして位置を補正
- 接地フラグをセット

**リスポーン**
- Y座標が-10以下になったら高い位置にリセット

## ビルドシステム

### CMake設定

**必須要件**
- CMake 3.15以上
- Windows 10/11
- Visual Studio 2019以上
- Windows SDK 10.0.19041.0以上

**ライブラリ依存**
- d3d12.lib: DirectX 12コアライブラリ
- dxgi.lib: DirectX Graphics Infrastructure
- d3dcompiler.lib: シェーダーコンパイラ

**ビルド成果物**
- 実行ファイル: DirectX12_3DPlatformer.exe
- シェーダー: shaders/*.hlsl（実行時コンパイル）

### ディレクトリ構造

```
3D_Game/
├── include/           # ヘッダーファイル
│   ├── DX12Core.h    # DirectX 12管理
│   ├── Player.h      # プレイヤーロジック
│   ├── Platform.h    # プラットフォームロジック
│   └── d3dx12.h      # DirectX 12ヘルパー（Microsoft提供）
├── src/              # 実装ファイル
│   ├── DX12Core.cpp  # DirectX 12実装
│   ├── Player.cpp    # プレイヤー実装
│   ├── Platform.cpp  # プラットフォーム実装
│   └── main.cpp      # エントリポイント、ゲームループ
├── shaders/          # HLSLシェーダー
│   ├── VertexShader.hlsl
│   └── PixelShader.hlsl
├── CMakeLists.txt    # CMakeビルド設定
├── build.bat         # ビルドヘルパースクリプト
├── README.md         # ユーザー向けドキュメント
├── claude.md         # このファイル（実装ドキュメント）
├── LICENSE           # MITライセンス
└── .gitignore        # Git除外設定
```

## パフォーマンス考慮事項

### GPU最適化

1. **インデックスバッファ使用**: 頂点の重複を削減
2. **定数バッファの効率的な更新**: 毎フレームmemcpyで更新
3. **リソースバリアの最小化**: 必要な遷移のみ実行
4. **デスクリプタヒープの事前確保**: 動的確保を避ける

### CPU最適化

1. **高精度タイマー**: QueryPerformanceCounterでデルタタイム計算
2. **メッセージポーリング**: PeekMessageで非ブロッキング
3. **デルタタイムクランプ**: 最大0.1秒に制限してスパイク対策

## 拡張可能性

### 追加機能の実装ガイド

**テクスチャマッピング**
1. D3D12_DESCRIPTOR_RANGE_TYPE_SRVをルートシグネチャに追加
2. サンプラーステートの作成
3. テクスチャリソースの作成とアップロード
4. シェーダーでテクスチャサンプリング

**ライティング**
1. 法線ベクトルを頂点データに追加
2. ライト情報を定数バッファに追加
3. ピクセルシェーダーでPhongライティング実装

**複数オブジェクト描画**
1. インスタンシング使用
2. または複数の描画コール（現在の実装）

**アニメーション**
1. ボーン行列を定数バッファに追加
2. 頂点ブレンディング実装
3. アニメーションシステムの構築

## トラブルシューティング

### よくある問題

**DirectX 12が初期化できない**
- デバッグレイヤーを有効化して詳細なエラーメッセージを確認
- GPUドライバーを最新版に更新
- Windows 10 バージョン1709以降を使用

**シェーダーコンパイルエラー**
- shaders/ディレクトリが正しい場所にあるか確認
- D3DCompileFromFile()のエラーメッセージを確認
- HLSL構文エラーをチェック

**画面が真っ黒**
- ビュー行列とプロジェクション行列が正しいか確認
- 深度テストが正しく設定されているか確認
- クリアカラーが適切か確認

**クラッシュ**
- デバッグビルドでスタックトレースを確認
- リソースの解放順序を確認
- フェンス同期が正しく動作しているか確認

## 参考資料

### DirectX 12学習リソース

1. **Microsoft公式ドキュメント**
   - [DirectX 12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
   - [DirectX Graphics Samples](https://github.com/microsoft/DirectX-Graphics-Samples)

2. **書籍**
   - "Introduction to 3D Game Programming with DirectX 12" by Frank Luna
   - "Real-Time Rendering, Fourth Edition"

3. **オンラインチュートリアル**
   - [Braynzar Soft DirectX 12 Tutorials](https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials)
   - [3dgep.com DirectX 12 Tutorials](https://www.3dgep.com/learning-directx-12-1/)

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。

## 今後の開発計画

### Phase 1: 基本機能（完了）
- ✅ DirectX 12初期化
- ✅ 基本的な3Dレンダリング
- ✅ プレイヤー移動とジャンプ
- ✅ プラットフォーム衝突判定
- ✅ 物理演算

### Phase 2: グラフィックス強化（未実装）
- テクスチャマッピング
- ライティングシステム
- シャドウマッピング
- パーティクルエフェクト

### Phase 3: ゲームプレイ拡張（未実装）
- レベルエディタ
- 敵キャラクター
- アイテムシステム
- スコアリング

### Phase 4: 最適化（未実装）
- マルチスレッドレンダリング
- フラスタムカリング
- LODシステム
- メモリプール

## 結論

このプロジェクトは、DirectX 12の基本的な使い方を学ぶための良い出発点となります。
低レベルなグラフィックスAPIの理解を深め、3Dゲーム開発の基礎を習得できます。

実装された機能は拡張可能な設計になっており、テクスチャ、ライティング、
より複雑なゲームロジックなどを追加することで、本格的な3Dゲームへと
発展させることができます。

---

**実装者**: Claude (Anthropic AI Assistant)
**実装日**: 2026年1月3日
**バージョン**: 1.0.0
