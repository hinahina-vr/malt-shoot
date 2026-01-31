# Malt-Shoot 🥃

モルトバトルちゃんねるの二次創作・縦スクロール弾幕シューティングゲーム

🔗 [YouTube](https://www.youtube.com/@maltbc)

## Features

- 🎮 **弾幕シューティング** - グレイズ、スペシャルゲージ、コンボシステム
- 🥃 **ウイスキーテーマ** - 樽、ボトル、グラスの敵キャラクター
- 🎆 **美しい弾幕パターン** - Flower, Rose, Wave, Ringなど
- 📊 **FPS表示** - リアルタイムパフォーマンス表示
- 🔊 **ひなひなボイス** - えーやお！、ぶおー

## Controls

| キー | 操作 |
|------|------|
| ↑↓←→ / WASD | 移動 |
| Z | ショット |
| X | ボム（敵弾消去） |
| Shift | 低速移動（当たり判定表示） |
| ESC | 設定メニュー |

### デバッグコマンド

| キー | 機能 |
|------|------|
| B | ボスモード開始 |
| R | ゲームリセット |
| P | フルパワー |
| G | ゲージ満タン |

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release --target MaltShoot
.\build\Release\MaltShoot.exe
```

## Credits

- **開発**: 能書き同好会
- **テーマ・ボイス**: モルトバトルちゃんねる
- **キャラ**: ひなひな（プレイヤー）、かい（ボス）

---

🥃 **Cheers!** 乾杯！
