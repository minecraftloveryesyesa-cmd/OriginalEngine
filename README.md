# Gasoline Engine

A cross-platform game-engine project, starting with a C++20 foundation.

## Current status

- CMake-based C++20 project
- SDL3 window and event loop
- First-person camera with WASD movement and mouse-look
- Axis-aligned collision world; camera movement is blocked by obstacles and slides along walls
- Basic scene registry with unique object names, transform data, visibility flags, lookup, and removal
- Wavefront OBJ loader for positions, optional UV coordinates and normals, polygon triangulation, negative indices, and per-corner attribute indexing
- Q/E vertical movement; Shift for faster movement
- Right-click toggles mouse capture; Esc releases the mouse or exits
- On-screen debug overlay showing smoothed FPS, camera coordinates, and control hints
- Near-plane clipping for cube faces and loaded mesh triangles, preserving the visible portion of geometry that crosses the camera's near plane
- Animated perspective-projected 3D cube with six face colors and all three rotation axes
- Renders multiple visible scene objects from shared OBJ mesh data, applying each object's position, rotation, scale, and visibility flag
- Press 1 or 2 to toggle the visibility of each demo pyramid at runtime
- Press F3 to toggle wireframe rendering for the cube and loaded OBJ meshes
- Filled triangles and floor-grid lines use a CPU-side Z-buffer for per-pixel visibility
- Software filled rendering is capped at 360,000 pixels per frame and scaled by SDL, preventing high-DPI windows and resizing from causing unbounded CPU and memory use
- World-space perspective floor grid that responds to camera movement and is occluded by nearer geometry
- GitHub Actions builds the Windows executable, runs camera, collision, scene-management, and OBJ-loader unit tests plus a rendering smoke test, and uploads the executable as an artifact

> The current renderer is an initial CPU-side projection and SDL rasterization prototype, not yet a modern GPU 3D pipeline. Filled triangles and floor-grid lines use a CPU-side depth buffer; wireframe mode is still drawn as SDL lines without depth testing. Geometry crossing the near plane is clipped instead of being discarded wholesale. To keep frame time and memory bounded when a shape fills the view or the window is high-DPI, filled rendering uses at most 800 x 450 pixels and SDL scales it to the window.

## Camera controls

- **W / S:** move forward / backward
- **A / D:** strafe left / right
- **Q / E:** move down / up
- **Left or right Shift:** move faster
- **1:** show or hide the right demo pyramid; **2:** show or hide the left demo pyramid
- **F3:** toggle wireframe mode for the cube and OBJ meshes
- Filled 3D geometry and floor-grid lines use a CPU-side Z-buffer for depth testing
- **Right mouse button:** capture/release the mouse for looking around
- **Mouse movement:** look around while captured
- **Esc:** release the mouse; press again to exit

The top-left debug overlay displays a smoothed FPS estimate, camera position (X/Y/Z), and a reminder of the main controls. F3 switches the cube and loaded OBJ meshes between filled polygons and wireframe outlines. Filled 3D triangles and floor-grid lines use a CPU-side Z-buffer, so nearer surfaces win per pixel instead of relying only on polygon draw order. Wireframe lines are not depth-tested yet. It uses SDL3's built-in debug text renderer, so no font or SDL_ttf asset is required.

## OBJ mesh loader prototype

`ObjMeshLoader` reads Wavefront OBJ positions (`v`), texture coordinates (`vt`), normals (`vn`), and faces (`f`), triangulates polygon faces, and supports positive and negative indices for each attribute. Vertices are split when a single position is referenced with different UV or normal indices. It validates face indices and leaves the caller's existing mesh unchanged on failure. The bundled `assets/demo.obj` pyramid is loaded once and instanced twice in the demo scene; each visible scene object is rendered with its own position, rotation, and scale. Scene objects marked invisible are skipped by the renderer. OBJ UV coordinates are used for perspective-correct per-pixel texture mapping when a texture is assigned to a mesh instance. The renderer currently supports ASCII P3 PPM textures and keeps the CPU Z-buffer and existing vertex lighting. MTL material parsing and PNG/JPEG/BMP decoding are not implemented yet. Triangles crossing the camera's near plane are clipped in camera space before projection.

## Collision prototype

The collision module supports static axis-aligned bounding boxes (AABBs). Camera movement resolves X, Y and Z separately to prevent entering a box and to allow sliding along its faces. This is a prototype; rotating meshes, slopes, stair stepping and a full capsule controller are not implemented yet.

## Requirements

- CMake 3.24+
- A C++20 compiler
- Git and internet access during the first configure (SDL3 is downloaded by CMake)

## Build on Windows

Run in a terminal with CMake and MSVC available:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\originalengine_app.exe
```

## Build through GitHub Actions (no local Windows PC required)

1. Open the repository's Actions tab.
2. Select the Build workflow and choose Run workflow, or push a commit to main.
3. Open the completed run and download the Gasoline Engine-Windows-x64 artifact.
4. Extract the ZIP and run originalengine_app.exe on a compatible Windows PC.

GitHub Actions can compile the Windows app and run automated tests, but it cannot show the interactive window inside the build log. You need a Windows device to run and visually inspect the executable.

## Project layout

```text
include/engine/       Public engine headers
src/engine/           Engine implementations
src/main.cpp          Sample application entry point
tests/                Automated camera, collision, and scene tests
.github/workflows/    GitHub Actions workflow
```

## Roadmap

1. Initial application and build foundation
2. First 3D projection and colored cube
3. First-person camera and movement
4. GPU-backed rendering and depth testing
5. Scene object instances and loaded mesh rendering
6. Texture/material loading, audio, and physics
7. Platform support expansion


---

# 日本語

Gasoline Engine は、C++20 を基盤として開発しているクロスプラットフォーム向けゲームエンジンのプロジェクトです。

## 現在の機能

- CMake を使用した C++20 プロジェクト
- SDL3 によるウィンドウ管理とイベントループ
- WASD 移動とマウス視点操作に対応した一人称カメラ
- 軸平行境界ボックス（AABB）による衝突判定。障害物への進入を防ぎ、壁に沿った移動が可能
- オブジェクト名の一意性、Transform、表示・非表示、検索、削除に対応した基本的なシーン管理
- Wavefront OBJ の頂点・UV 座標・法線・面データ読み込み。多角形の三角形分割と各属性の負のインデックスに対応
- Q/E キーによる上下移動、Shift キーによる高速移動
- 右クリックでマウス捕捉を切り替え、Esc キーで捕捉解除または終了
- FPS、カメラ座標、操作ヒントを表示するデバッグ HUD
- カメラの近接平面を横切るキューブや読み込み済みメッシュのポリゴンをクリッピングし、見える部分を描画
- 3 軸で回転する、6 色の面を持つ透視投影 3D キューブ
- 同じ OBJ メッシュデータを複数のシーンオブジェクトで共有し、それぞれの位置・回転・拡大縮小・表示状態を反映して描画
- 1 キーと 2 キーでデモ用ピラミッドの表示・非表示を切り替え
- カメラの移動に応じて見え方が変わり、近いメッシュに隠れる、ワールド座標系の透視投影フロアグリッド
- ソフトウェア描画は 1 フレームあたり 360,000 ピクセルに制限され、SDL により拡大表示されるため、高 DPI ウィンドウやリサイズで CPU・メモリ使用量が無制限に増加しない
- 塗りつぶしポリゴンとフロアグリッド線に対応した CPU 側 Z バッファによるピクセル単位の奥行き判定
- GitHub Actions による Windows 実行ファイルのビルド、カメラ・衝突判定・シーン管理・OBJ ローダーのユニットテスト、描画スモークテスト、および成果物のアップロード

> 現在の描画機能は、CPU 側で透視投影を行い、SDL で三角形をラスタライズする初期段階の試作です。まだ最新の GPU 3D パイプラインではありません。奥行きの判定にはペインターアルゴリズムを使用しているため、交差するポリゴンや複雑なシーンでは制限があります。将来的には Z バッファによる奥行き判定が必要です。近接平面を横切るポリゴンは、全体を破棄せずにクリッピングしてから描画します。

## カメラ操作

- **W / S：** 前進 / 後退
- **A / D：** 左右に平行移動
- **Q / E：** 下 / 上に移動
- **左または右 Shift：** 高速移動
- **1：** 右側のデモ用ピラミッドを表示 / 非表示
- **2：** 左側のデモ用ピラミッドを表示 / 非表示
- **マウス右ボタン：** 視点操作用のマウス捕捉を切り替え
- **マウス移動：** マウス捕捉中に視点を操作
- **Esc：** マウス捕捉を解除。もう一度押すと終了

画面左上のデバッグ HUD には、平滑化した FPS の推定値、カメラ座標（X/Y/Z）、主な操作方法が表示されます。塗りつぶしポリゴンとフロアグリッド線は CPU 側の Z バッファで奥行き判定され、手前のジオメトリが奥のグリッド線を隠します。ワイヤーフレーム線はまだ深度判定されません。SDL3 標準のデバッグ文字描画機能を使うため、フォントファイルや SDL_ttf は必要ありません。

## OBJ メッシュローダー（試作）

`ObjMeshLoader` は Wavefront OBJ の `v`（頂点）と `f`（面）レコードを読み込みます。多角形の面を三角形に分割し、正の頂点インデックスと負の頂点インデックスの両方に対応します。OBJ の UV 座標を使った透視補正付きのピクセル単位テクスチャ描画に対応しました。現時点では ASCII 形式の P3 PPM テクスチャを読み込み、CPU 側の Z バッファと既存の頂点ライティングを維持します。MTL マテリアル解析や PNG/JPEG/BMP のデコードは未実装です。面のインデックスを検証し、読み込みに失敗した場合は呼び出し元の既存メッシュを変更しません。

同梱の `assets/demo.obj` にあるピラミッドは一度読み込まれ、デモシーン内の 2 つのオブジェクトで共有されます。表示状態のオブジェクトは、それぞれの位置・回転・拡大縮小を反映して描画されます。非表示に設定されたオブジェクトは描画されません。OBJ の UV 座標を使った透視補正付きのピクセル単位テクスチャ描画に対応しました。現時点では ASCII 形式の P3 PPM テクスチャを読み込み、CPU 側の Z バッファと既存の頂点ライティングを維持します。MTL マテリアル解析や PNG/JPEG/BMP のデコードは未実装です。カメラの近接平面を横切る三角形は、投影の前にカメラ座標系でクリッピングされます。

## 衝突判定（試作）

衝突判定モジュールは、静的な軸平行境界ボックス（AABB）に対応しています。カメラの移動を X・Y・Z 軸ごとに解決することで、ボックス内部への侵入を防ぎ、面に沿って滑るように移動できます。回転したメッシュとの衝突、斜面、階段昇降、カプセル形状を使った本格的なキャラクターコントローラーは未実装です。

## 必要なもの

- CMake 3.24 以降
- C++20 対応コンパイラ
- Git と初回構成時のインターネット接続（CMake が SDL3 をダウンロードします）

## Windows でビルドする

CMake と MSVC が使えるターミナルで、次のコマンドを実行します。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\\build\\Release\\originalengine_app.exe
```

## GitHub Actions でビルドする（Windows PC が手元になくても可）

1. GitHub リポジトリの **Actions** タブを開きます。
2. **Build** ワークフローを選び、**Run workflow** を実行するか、`main` ブランチにコミットを push します。
3. ワークフローの実行が完了したら、成果物 `Gasoline Engine-Windows-x64` をダウンロードします。
4. ZIP を展開し、対応する Windows PC で `originalengine_app.exe` を実行します。

GitHub Actions は Windows アプリのビルドと自動テストを実行できますが、ビルドログ内に操作可能なゲームウィンドウを表示することはできません。実際の見た目を確認するには、Windows 端末で実行する必要があります。

## ディレクトリ構成

```text
include/engine/       公開用エンジンヘッダー
src/engine/           エンジンの実装
src/main.cpp          サンプルアプリケーションのエントリーポイント
tests/                カメラ・衝突判定・シーンなどのテスト
.github/workflows/    GitHub Actions のワークフロー
```

## 開発ロードマップ

1. アプリケーションとビルド環境の基礎
2. 3D 透視投影とカラーキューブ
3. 一人称カメラと移動操作
4. GPU 描画と奥行き判定
5. シーンオブジェクトと読み込んだメッシュの描画
6. テクスチャ / マテリアル、音声、物理演算
7. 対応プラットフォームの拡張
