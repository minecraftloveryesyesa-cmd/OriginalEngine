# GasolineEngine — AI開発引き継ぎ資料

> この文書は、別のAI・開発者が会話履歴なしで作業を再開できるようにするための、リポジトリ基準の引き継ぎ資料です。
> **最初に必ずGitHubの対象ブランチと実ファイルを再確認してください。** この文書は調査時点のスナップショットであり、コードより優先されません。

- Repository: https://github.com/shton13275-2/GasolineEngine
- 開発対象ブランチ: `ios/initial-support`
- 直近の引き継ぎ資料更新前に確認したHEAD: `7c8c1a1a89f58cf883ef89b572fa7a6b3373916c`（この資料の更新後はGitHubで最新HEADを再確認すること）
- デフォルトブランチ: `main`（iOS作業ブランチとは別系統）
- 言語: C++20
- ウィンドウ・入力・GPU API: SDL 3.2.10（CMake FetchContent、`release-3.2.10`）
- ビルド: CMake 3.24+ / GitHub Actions
- ユーザーの主な実機: iPhone。Macを常用できず、GitHub ActionsのmacOS runnerでiOSビルドする。未署名IPAをLiveContainerで試す運用。
- 重要: ユーザーは「確認済みの事実だけ」を希望。推測を事実のように書かない。絵文字は使わない。会話は堅くしすぎず簡潔に。

## 目次

1. プロジェクトの目的
2. ブランチ・作業対象の注意
3. 現在実装されている機能
4. GPUバックエンドの正確な状態
5. iOS対応・ビルド・配布
6. ディレクトリと主要ファイル
7. 操作方法
8. 未実装・既知の制限
9. ロードマップと優先順位
10. 開発・検証ルール
11. AIエージェント向け作業手順
12. 完了報告の形式
13. 参考リンク

---

## 1. プロジェクトの目的

GasolineEngineは、C++20とSDL3を使ったクロスプラットフォーム3Dゲームエンジンの試作プロジェクト。

長期目標:
- iOSとWindowsを含む複数プラットフォームで動く3Dエンジンにする。
- iPhone上で操作できるデモを維持する。
- 現在のCPUソフトウェアレンダラーを維持しつつ、SDL_GPUを使う本格的なGPUレンダラーを実装する。
- GPU/CPU描画を実行時に切り替えられるようにする。
- 最終的にはOBJメッシュ、深度、テクスチャ、シーンオブジェクトをGPUで描画する。
- パフォーマンス、操作性、安定性を段階的に改善する。

重要な現状認識:
- CPUソフトウェアレンダラーは既に多くの機能を持つ。
- GPU経路は試験的な段階。GPUデバイス・スワップチェーン・三角形プレビューがあるが、既存3DシーンをGPUで描画する完成レンダラーではない。
- GPUモードで表示される三角形は、OBJモデルの描画ではなく、頂点IDから生成する固定プレビュー。
- 「GPU描画対応済み」「Metalシェーダーが実際にロードされる」といった表現は、コードと実機で確認できるまでは使わない。

## 2. ブランチ・作業対象の注意

GitHubには少なくとも次のブランチが存在する。

| ブランチ | 役割 |
|---|---|
| `main` | デフォルトブランチ。CPUレンダラー中心の基礎版 |
| `ios/initial-support` | iOS対応、未署名IPA、タッチ操作、GPU実験を進めている開発対象 |

**通常の継続作業は `ios/initial-support` を対象にする。** `main` に誤ってコミットしないこと。作業開始時に必ずブランチHEADを読み直す。

調査時点で確認した差:
- `main` のREADMEはWindows中心の初期CPUレンダラー説明。
- `ios/initial-support` のREADMEにはiOS Device unsigned IPA、タッチ操作、GPUスモークテストの説明がある。
- 両ブランチのコードとREADMEを混ぜないこと。

ブランチHEADがこの文書記載のSHAと違う場合は、新しいHEADを正としてファイルを再確認する。

## 3. 現在実装されている機能

以下は `ios/initial-support` のREADMEおよびソースを調査して確認した内容。

### エンジン基盤
- CMakeベースのC++20プロジェクト。
- SDL3によるウィンドウ、イベント、入力、描画表示。
- 一人称カメラ。
- カメラ移動と視点操作。
- デバッグログ。iOSではSDLのPreference Path配下に `startup.log` を作る実装。
- 起動時のエラーをログに書き、メッセージボックスでも通知する実装。

### カメラ・操作
- Windows/PC: WASD移動、Q/E上下移動、Shift高速移動。
- 右クリックでマウス捕捉、マウス移動で視点変更。
- Escでマウス捕捉解除、解除済みなら終了。
- Rでカメラリセット。
- Spaceでデモアニメーション一時停止/再開。
- F3でワイヤーフレーム切替。
- F11でウィンドウ/フルスクリーン切替。
- 1/2でデモ用ピラミッドの表示切替。
- -/+でマウス感度調整。
- iOS: 左半分のドラッグ領域で移動、右半分のドラッグ領域で視点操作。両方同時操作できる設計。
- iOS上のGPU/CPU切替ボタンはGPU/CPUバックエンドの切替要求に結び付けられている。ボタンの表示・タッチ判定の細部は最新コードで再確認すること。

### シーン・メッシュ
- 名前が一意なシーンオブジェクト登録。
- Transform（位置・回転・スケール）。
- 表示/非表示、検索、削除。
- Wavefront OBJローダー。
- OBJの位置 `v`、UV `vt`、法線 `vn`、面 `f` の読み込み。
- 多角形面の三角形分割。
- 正/負インデックス。
- 同じ位置でもUV/法線インデックスが異なる場合の頂点分離。
- 読み込み時のインデックス検証。失敗時に呼び出し元の既存メッシュを破壊しない設計。
- `assets/demo.obj` のピラミッドを読み込み、2つのシーンオブジェクトで共有するデモ。

### CPUソフトウェアレンダラー
- 透視投影。
- キューブの3軸回転デモ。
- 複数OBJインスタンス描画。
- カメラ近接平面を横切るポリゴンのクリッピング。
- CPU側Zバッファによる塗りつぶし三角形のピクセル単位深度判定。
- フロアグリッドの描画と深度判定。
- 透視補正付きUVテクスチャマッピング。
- ASCII P3 PPMとBMPテクスチャの読み込み（iOSブランチREADMEの記述。実装と一致するか拡張時に再確認）。
- ソフトウェア描画サイズに上限を設け、SDLでウィンドウサイズに拡大表示する。
- READMEには最大800×450（360,000ピクセル）と記載。定数変更時はREADMEも更新。
- デバッグHUDにFPS推定値、カメラ座標、操作ヒントを表示。
- ワイヤーフレームもCPU側Zバッファで深度判定するとのiOSブランチREADME記載。古いmainブランチREADMEにはワイヤーフレーム深度判定なしとあるため、対象ブランチのコードを優先する。

### 衝突判定
- 静的AABB（軸平行境界ボックス）。
- X/Y/Z軸ごとに移動を解決。
- 障害物への侵入を防ぎ、面に沿ってスライド。
- 回転メッシュ、斜面、階段、カプセルコントローラーは未実装。

## 4. GPUバックエンドの正確な状態

### 既に存在するコード
- `src/main.cpp` にSDL_GPU初期化、ウィンドウClaim、スワップチェーン取得、GPUコマンド送信処理がある。
- `--gpu-smoke-test` はGPUデバイスを作り、ウィンドウをClaimし、swapchain textureを取得し、Render Passでクリアし、コマンドをSubmitする。
- `initializeGpuBackend()` はGPUデバイスを作り、SDL_ClaimWindowForGPUDeviceでウィンドウをClaimする。
- `createGpuTrianglePipeline()` はMSLソースを文字列として保持し、SDL_CreateGPUShaderで頂点/フラグメントシェーダーを作り、Graphics Pipelineを作る。
- 頂点シェーダーは `vertex_id` から3頂点の位置と色を生成する。頂点バッファは使わない。
- `renderGpuPreviewFrame()` はswapchain textureを取得し、Render Passを開始し、PipelineをBindして `SDL_DrawGPUPrimitives(..., 3, 1, 0, 0)` を呼ぶ。
- GPU/CPU切替処理があり、GPU有効化に失敗した場合はCPU rendererを再生成するフォールバック経路がある。
- GPUデバイス、Pipeline、Window Claimを終了時に解放する処理がある。
- `src/engine/GpuDeviceProbe.cpp` と `include/engine/GpuDeviceProbe.hpp` に、GPUデバイスの利用可否・driver名・shader formatを調べるProbeクラスがある。ただし、ProbeはウィンドウをClaimせず、描画機能そのものではない。

### Metal shader files
- `shaders/Metal/Basic3D.vert.metal`
- `shaders/Metal/Basic3D.frag.metal`
- iOS CMake設定で `Resources/shaders` に同梱する。
- **現状のGPU三角形Pipelineは、これらのファイルを読まず、main.cpp内のMSL文字列を使う。**
- 外部ファイルのシェーダーは、現時点ではリソースとして追加されているだけ。実行時ロード・コンパイルに接続済みだと誤認しない。
- シェーダーの頂点入力属性・Uniform設計はあるが、GPUパイプラインへの実接続は未完了。

### GPUで未実装の重要機能
- 既存OBJメッシュをGPU頂点/インデックスバッファへアップロード。
- モデル/ビュー/投影行列のGPU送信。
- シーン内複数オブジェクトのGPU描画。
- 深度テクスチャ/Depth-Stencil AttachmentによるGPU深度テスト。
- GPUテクスチャ作成・アップロード・サンプリング。
- GPU側のライティング。
- CPUとGPUで同じシーンを描画する共通Renderer API。
- GPUとCPUの画質・機能差の縮小。
- iPhone実機でのGPU三角形描画の確認。
- GPU切替を繰り返したときのリソースリーク/クラッシュ確認。
- GPUのFPS・フレーム時間の計測。

### 重要な注意
- GPUモードで画面が単色になるだけの旧段階と、現在の三角形プレビュー段階を混同しない。
- 三角形が表示できても、3Dメッシュ描画ができたことにはならない。
- CIでiOS IPAが作れたことは、iPhone上のMetal実行成功を意味しない。
- `SDL_GPU_SHADERFORMAT_MSL` はSDL_GPUの対応フォーマット指定の一つ。実際のSDL3のAPI仕様は、FetchContentで使うSDL 3.2.10のヘッダーを参照して判断する。
- SDL_GPUのリソース所有権、Render Pass、Swapchain、Submit/Cancelの規則に従う。Swapchain texture取得後に安易にCommandBufferをCancelしない。失敗経路はSDL3の該当バージョン仕様を確認する。

## 5. iOS対応・ビルド・配布

### iOSビルド
- GitHub ActionsのmacOS runnerでXcode/CMakeを使用。
- iOS Device arm64。
- Deployment Target: iOS 15.0。
- コード署名なしで.appをビルドし、`Payload/` に入れてunsigned IPAを作成。
- IPAをzipし、ZIP integrityとInfo.plist、実行ファイルの存在を検証。
- 署名なしIPAは通常のApp Store配布用ではない。ユーザーはLiveContainerでテストする運用。
- LiveContainerで起動できた過去実績があるが、毎回の新しいIPAで動作する保証はない。新ビルドごとに実機テストする。

### iOS CMake
- `CMAKE_SYSTEM_NAME=iOS`
- `CMAKE_OSX_SYSROOT=iphoneos`
- `CMAKE_OSX_ARCHITECTURES=arm64`
- `CMAKE_OSX_DEPLOYMENT_TARGET=15.0`
- iOSではSDL_SHARED=OFF、SDL_STATIC=ON。
- `SDL3::SDL3-static` にリンク。
- App Bundle identifier: `com.gasolineengine.demo`
- Bundle name: `Gasoline Engine`
- `SDL_main.h` をmain.cppでincludeすること。過去にSDL_Initのmain entrypoint関連エラーが発生した。
- iOS向けにOBJ/PPM資産とMetal shader sourceをResourcesに配置する設定がある。

### Workflow
- `.github/workflows/build.yml`
- Windows/MSVC job:
  - Configure CMake
  - Build Release
  - `--smoke-test`
  - CTest
  - Windows artifact upload
- iOS unsigned IPA job:
  - macOS runner
  - Xcode generatorでiOS Device arm64 build
  - unsigned .appをIPA化
  - IPA integrity / bundle contents verify
  - artifact upload
- 実際のworkflow runとjob結果は作業のたびにGitHub Actionsで確認する。古い成功runを最新変更の成功として扱わない。

## 6. ディレクトリと主要ファイル

| Path | 役割 |
|---|---|
| `CMakeLists.txt` | C++20、SDL3 FetchContent、Windows/iOSターゲット、テスト |
| `src/main.cpp` | デモアプリ、入力、iOSタッチ、GPU/CPU切替、GPUプレビュー |
| `include/engine/Camera.hpp` / `src/engine/Camera.cpp` | カメラ |
| `include/engine/CollisionWorld.hpp` / `src/engine/CollisionWorld.cpp` | AABB衝突 |
| `include/engine/Scene.hpp` / `src/engine/Scene.cpp` | シーンとTransform |
| `include/engine/Mesh.hpp` | MeshVertex/MeshTriangle/Mesh |
| `include/engine/ObjMeshLoader.hpp` / `src/engine/ObjMeshLoader.cpp` | OBJロード |
| `include/engine/Renderer3D.hpp` / `src/engine/Renderer3D.cpp` | CPUソフトウェアレンダラー |
| `include/engine/GpuDeviceProbe.hpp` / `src/engine/GpuDeviceProbe.cpp` | SDL_GPU利用可否のProbe |
| `shaders/Metal/` | iOS向けMSL shader source（まだmain.cppのGPU previewからは未使用） |
| `shaders/Source/` | HLSL shader source |
| `assets/demo.obj` | デモ用ピラミッド |
| `assets/demo_texture.ppm` | デモ用テクスチャ |
| `tests/` | カメラ、衝突、シーン、OBJ、Renderer等のテスト |
| `.github/workflows/build.yml` | Windows build/testとiOS unsigned IPA |

## 7. 操作方法

### Windows / PC
| 入力 | 操作 |
|---|---|
| W/S | 前進/後退 |
| A/D | 左右移動 |
| Q/E | 下/上 |
| Shift | 高速移動 |
| 右クリック | マウス捕捉切替 |
| マウス移動 | 視点操作（捕捉中） |
| Esc | 捕捉解除 / 終了 |
| R | カメラリセット |
| Space | デモアニメーション停止/再開 |
| F3 | ワイヤーフレーム切替 |
| F11 | フルスクリーン切替 |
| 1/2 | ピラミッド表示切替 |
| -/+ | マウス感度 |

### iOS
- 画面左半分のドラッグ: 仮想ジョイスティックによる移動。
- 画面右半分のドラッグ: 視点操作。
- 左右を同時に操作可能な設計。
- 画面上のGPU/CPU切替ボタン: 描画バックエンド切替。現在GPU側はプレビュー段階。

## 8. 未実装・既知の制限

- GPU rendererはCPU rendererと同じ機能を持たない。
- GPUの三角形プレビューはOBJ/Scene/Cameraを使わない。
- Metal shader sourceファイルは同梱されるが、現在のGPU preview pipelineはmain.cpp内のMSL文字列を使う。
- GPU depth bufferは未実装。
- GPU mesh vertex/index buffer uploadは未実装。
- GPU texture samplingは未実装。
- GPU shader external asset loadingは未実装。
- MTL material parserは未実装。
- PNG/JPEG decoderは未実装（現行READMEにあるPPM/BMP対応を維持）。
- AABB以外の本格的な物理・衝突は未実装。
- iOSの物理端末GPU検証はCIだけではできない。
- 未署名IPAは端末への通常インストールを保証しない。現在のユーザー環境で使えるLiveContainer等の方法をユーザーの実績に沿って確認する。

## 9. ロードマップと優先順位

### P0 — 既存機能とCIを壊さない
1. 作業開始時にブランチ/HEAD/差分を確認。
2. Windows jobとiOS unsigned IPA jobの両方を成功させる。
3. CPU rendererへのフォールバックを維持。
4. GPU切替失敗、初期化失敗、終了時のリソース解放をテスト。

### P1 — GPUプレビューを堅牢化
1. GPUプレビュー三角形がiOS実機で表示されることを確認。
2. GPU/CPU切替を繰り返してもクラッシュしないことを確認。
3. SDL_GPUのresource lifecycleとswapchainサイズ変更を処理。
4. GPU backendの診断ログを充実させる。
5. GPU smoke testを可能な範囲で自動テストに組み込む。

### P2 — 実シーンをGPU描画
1. GPU専用Rendererクラスを作り、main.cppからGPU API詳細を分離。
2. MeshVertexをGPU頂点形式へ変換。
3. 頂点/インデックスバッファを作成し、GPUへアップロード。
4. Camera/TransformからMVP行列を計算してuniform送信。
5. デモ三角形からOBJピラミッドへ移行。
6. 複数SceneObjectのTransformとvisibleをGPU描画へ反映。

### P3 — GPU深度・テクスチャ
1. Depth-Stencil Textureを作成。
2. Depth test/writeを有効化。
3. PPM/BMP画像をSDL_GPUTextureへ転送。
4. samplerとUV補間を実装。
5. CPU rendererとの画質差を比較し、フォールバックを維持。

### P4 — 品質向上
- MTL読み込み。
- PNG/JPEG等の画像形式。
- ライティング、法線、マテリアル。
- FPS/frame time表示とGPU/CPU比較。
- 音声・物理演算。
- プラットフォーム追加。

順序は依存関係に応じて調整してよい。ただしP0を壊さないこと。

## 10. 開発・検証ルール

ユーザーが特に重視するルール:

1. **事実確認を優先。** ソースで確認できないことを実装済みと書かない。
2. **コードを読んでから編集。** 既存コードを推測で置き換えない。
3. **小さい差分。** 一度に大量変更せず、ビルド/テストしやすい単位で進める。
4. **CPU rendererを壊さない。** GPUは別経路で追加し、失敗時はCPUに戻せるようにする。
5. **iOSを最優先の実機対象にする。** Windows CIも維持する。
6. **未署名IPAを直接生成する。** ZIPだけを成果物として渡すのではなく、artifact内に.ipaを生成する。
7. **CI成功と実機成功を区別。** CI成功はコンパイル/パッケージ確認であり、iPhoneのMetal描画成功ではない。
8. **コミットの有無を正直に報告。** commit SHAとリンクを確認できた場合のみ「コミット済み」と言う。
9. **ビルド結果を実際に確認。** 最新commit SHAとworkflow runのhead_shaが一致しているか確認。
10. **失敗を隠さない。** workflow failure時は失敗job/stepを特定し、原因が分からなければ未特定とする。
11. **ユーザーに不要な確認を何度も求めない。** リポジトリ情報から分かることは自分で調べる。
12. **バックアップと既存動作を守る。** 大きな変更は別branch/commitに分ける。
13. **ドキュメントも更新。** 実装内容・操作・制限が変わったらREADME/この資料を更新。
14. **絵文字なし。** 会話は自然な日本語で、過度に堅い表現にしない。
15. **引き継ぎ資料を継続更新。** 機能・設計・ビルド設定・CI結果・既知の問題・次の作業が変わったら、作業と同じコミット、または直後のコミットで `docs/AI_HANDOFF.md` を更新する。未確認事項は未確認と明記し、古い状態・完了済みタスク・古いHEAD情報を放置しない。

### 変更前チェック
- `git branch --show-current` 相当のGitHub branch確認
- 最新HEAD SHA確認
- 対象ファイルの現在内容を取得
- 関連APIは実際に使っているSDL 3.2.10のヘッダーで確認
- 既存のCPU経路、iOS bundle、Windows CIへの影響を検討

### 変更後チェック
- CMake configure/build
- CTest
- Windows smoke test
- iOS unsigned IPA build/package/verify
- GitHub Actions runのcommit SHA確認
- 可能ならiPhone/LiveContainerで実機テスト
- READMEとこの資料を更新

## 11. AIエージェント向け作業手順

新しいAIは、以下を順番に行うこと。

### Step 1: 状態確認
- リポジトリ `shton13275-2/GasolineEngine` を開く。
- `ios/initial-support` をcheckout対象にする。
- branch HEADを取得。
- `git status` 相当で未コミット変更を確認。
- この文書とREADMEを読む。
- `src/main.cpp`、`Renderer3D.cpp`、`GpuDeviceProbe.cpp`、CMake、workflowを読む。
- ここに書かれた説明と現行コードが異なる場合、コードを正とし、この文書を修正する。

### Step 2: 既存テストと直近CI
- 最新workflow runを確認。
- 最新runのhead_shaとbranch HEADが一致するか確認。
- Windows/iOS各jobの結果を確認。
- 失敗があれば実装に入る前に原因を特定する。

### Step 3: 次の作業を一つ選ぶ
推奨する次の作業は、GPU描画コードをmain.cppから独立したクラスへ分離し、既存GPU三角形プレビューを移すこと。分離前に現行の動作を維持するテストを設計する。
その後、GPU vertex/index bufferとcamera matrixを実装し、デモOBJを描画する。

### Step 4: 実装
- SDL 3.2.10の実際のAPIヘッダーを確認してから使う。
- MSLを使う場合はSDL_GPUが期待する形式、entrypoint、shader stage、resource bindingを確認。
- shader source fileがbundleにあるだけで実行時ロードできたと判断しない。
- まず最小三角形を実機で確認し、次にOBJへ進む。
- GPU resourceは所有者を明確にし、失敗時もすべて解放する。
- Window Claim中にSDL_Rendererを使わない。切替時のrenderer/device/window所有権を明確にする。
- GPUが利用できない場合CPUへ戻る経路を維持する。

### Step 5: コミット・検証
- 小さい変更単位でcommit。
- workflow runが新commitに紐づいているか確認。
- CIが成功するまで「完成」と言わない。
- 実機で未確認ならその旨を明記する。
- 完了報告にcommit SHA、CI URL、テスト結果、実機確認の有無を記載。

### Step 6: ユーザーへの報告
日本語で短く、以下を明確に:
- 何を変更したか
- commit済みか（SHA付き）
- CIの状態（成功/失敗/実行中）
- iPhone実機で確認したか
- 次の課題

## 12. 完了報告の形式

例:

```text
変更:
- GPU rendererを独立クラスへ分離
- CPU fallbackを維持

Commit:
- <SHA>
- <GitHub commit URL>

CI:
- Windows: success
- iOS unsigned IPA: success
- Artifact: <workflow URL>

実機:
- iPhone / LiveContainer: 未確認（または確認済みの具体的結果）

次:
- OBJ vertex/index buffer upload
```

「CI成功」「IPA生成成功」「iPhoneで起動成功」「GPUでOBJ描画成功」は別々の判定項目である。まとめて一つの「成功」としない。

## 13. 参考リンク

- Repository: https://github.com/shton13275-2/GasolineEngine
- iOS branch: https://github.com/shton13275-2/GasolineEngine/tree/ios/initial-support
- Actions: https://github.com/shton13275-2/GasolineEngine/actions
- SDL 3.2.10 GPU API header: https://github.com/libsdl-org/SDL/blob/release-3.2.10/include/SDL3/SDL_gpu.h
- SDL_shadercross: https://github.com/libsdl-org/SDL_shadercross
- SDL GPU examples: https://github.com/TheSpydog/SDL_gpu_examples
- GitHub README guidance: https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-readmes

---

## 追記履歴

- 2026-10-31: `ios/initial-support` のソース、README、CMake、workflow、GPU実装を確認して初版作成。GPU三角形プレビュー、iOS unsigned IPA workflow、CPU software renderer、iOSタッチ操作の状態を整理。
- 2026-10-31: iOS buildのSDL3 source cache、Xcode versionを含むcache key、`FETCHCONTENT_UPDATES_DISCONNECTED`、`.app`自動検出を追記。Windows/iOS両jobで`docs/`をsparse checkout対象外にした設定を追記。\n- 2026-10-31: Actionsの失敗runを調査。Xcode version出力が複数行のため`$GITHUB_OUTPUT`で不正形式になっていた問題、およびCMake経由の`xcodebuild`に`-derivedDataPath`を渡してscheme要求エラーになっていた問題を確認。workflowを修正し、Xcode versionを1行化、DerivedData cache/引数を一旦除去してSDL3 source cacheのみ残した。修正後CIの結果は未確認。
