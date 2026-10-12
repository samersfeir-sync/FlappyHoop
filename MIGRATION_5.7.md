# FlappyHoop: UE 5.4 → 5.7 migration and refactor notes

## Build the project (do this first)
1. Commit or stash in git, so you can diff everything against the pre-refactor state.
2. Delete `Binaries/`, `Intermediate/`, `.vs/` and `FlappyHoop.sln`.
3. Right-click `FlappyHoop.uproject` → *Generate Visual Studio project files* (pick 5.7).
4. Build **Development Editor | Win64** in VS 2022 (MSVC 14.44 is already in `.vsconfig`).
5. Open the editor and **compile these Blueprints**: BP_BaseGameMode, BP_GameInfoInstance, all widget BPs. Then *File → Save All*.
6. Android: install **SDK Platform 36** and **NDK r27c** in Android Studio (target SDK is now 36; Google Play has required this since 31 Aug 2026).

## Blocker fixed
Monetization Goodies 2.2.3 for 5.7 uses Play Billing 8, which **removed `QueryPurchaseHistory`**. The old
`GameInfoInstance.cpp` would not compile. Restoring purchases now uses `QueryPurchases`.

## Build config
- `IncludeOrderVersion` / `DefaultBuildSettings` → `Latest`.
- Build.cs now lists `UMG`, `Slate` and `SlateCore` explicitly. These were only pulled in by accident before.

## Bugs fixed
| Area | Bug |
|---|---|
| Ads | Interstitial loaded every 15s in the menu and could pop up **mid-game**. Now it's preloaded and shown on return to menu every N games (default 3) with a 60s cooldown. |
| Ads | Closing the rewarded ad without the reward **soft-locked** the game (popup hidden, nothing ended the run). Now it ends the run cleanly. |
| Ads | Late rewarded-ad callbacks after going Home could act on the menu. |
| Billing | Reconnect check was inverted, so it **never reconnected**. Now it retries with backoff. |
| Billing | Restored "remove ads" was never saved. The banner could still show. |
| Billing | Paid-but-unconsumed gem packs (app killed mid-purchase) were never credited. The product then stayed "already owned" forever. |
| Billing | Gems were credited based on the last button tapped, not on the actual purchased product. Billing is now handled in the GameInstance with queued ack/consume. |
| Gameplay | `GetActorOfClass(ACoins)` could return the **Gem** actor (AGems derives from ACoins). |
| Gameplay | Screen edge teleported the ball on *any* overlap. |
| Gameplay | Coins with no spawn points crashed (index -1). |
| Gameplay | A pending "switch sides" timer could fire into the next run. |
| Gameplay | Null deref in `Hoop::SwitchSides`. |
| UI | Shop kept an ever-growing list of stale item widgets (raw pointers, GC crash risk). Gem packs were duplicated on re-construct. |
| UI | Combo animation callback was re-bound on every combo. |
| UI | Second-chance timer overwrote the editor value with a hard-coded 12. The Gem button was re-enabled even when you couldn't afford it. |
| UI | `UTextBlock::Font` direct access is deprecated; now uses `GetFont`/`SetFont`. |
| Audio | Music component was registered before a world existed. There was no music in PIE because the startup-movie callback never fired there. |
| Save | Synchronous save on every gem pickup replaced with serialized async saves. New shop balls added in the BP now merge into existing saves. |
| Safety | Raw `UObject*` members are now `UPROPERTY TObjectPtr`. `GetWorldFromContextObjectChecked` is replaced with a null-safe version. On-screen debug spam is now `UE_LOG(LogFlappyHoop)`. |

## New gameplay
- **Swish:** a basket with no rim/backboard contact scores ×2 (`SwishScoreMultiplier` on the GameMode). It shows "Swish!" in the combo text. Optional `SwishSound` on the Hoop BP.
- **Space bar** launches the ball on PC/editor for quick testing.

## New tunables (all optional, with sensible defaults)
- GameMode: `InterstitialEveryNGames`, `InterstitialCooldownSeconds`, `InterstitialRetrySeconds`, `SwishScoreMultiplier`, `MinGameTime`.
- Hoop: `SwishSound`, `SwishContactWindow`.
- Settings widget: `SettingsSoundMix`. Set it to make SFX muting use a Sound Mix instead of editing the SoundClass asset.
- ShopItem widget: `DefaultOutlineColor`.

## Things to check / known follow-ups
- **Swish detection** relies on the ball's sweep being *blocked* by HoopMesh/HoopRing, the same mechanism the Ground uses. If swishes fire on every basket, check that those meshes block the ball's channel.
- **Security:** `DefaultEngine.ini` contains the keystore password in plain text and is in git. Consider moving it out of the repo.
- Gem pack prices are hard-coded in `GemShopInfo`. Play Store localized prices (`FormattedPrice`) could be shown instead.
- Purchases are not server-verified.

## Second chance widget (ported from the other game)
Round countdown ring (green → yellow → red), gem cost on the button, spinner swapped with the Watch Ad icon,
optional PopupShow/PopupHide animations. The widget BP needs: `CountdownRing` (Image using the ring material with
`Percent` + `Color` params), `WatchAdImage` (Image), `GemButtonText` (TextBlock). `MainText` is now optional.

## Refactor 2: folders, run phases, economy
- Source is now `Public/<Category>/*.h` + `Private/<Category>/*.cpp` (Core, Framework, Gameplay, Data, Systems, UI/HUD, UI/Popups, UI/Shop). Includes use `"Category/File.h"`. Class names unchanged, so BPs keep their parents.
- `EGameStateEnum` = MainMenu / Playing / SecondChance / GameOver. `ABaseGameMode` owns every transition (StartRun, ReturnToMenu, EndGame, second chance) and broadcasts `OnRunPhaseChanged`; widgets react instead of being called.
- Round timer moved from `GameWidget` into `ABaseGameMode::Tick` (pauses with the game, freezes during the basket celebration). HUD bar just displays `GetRoundTimeFraction()`.
- Hoop → `NotifyBasketPending` / `ScoreBasket(bSwish)`; Ground → `NotifyBallLanded`; controller asks `CanLaunchBall()` (no more enable/disable-input juggling).
- `UEconomySubsystem` (Systems): `AddCoins/AddGems/TrySpend/CanAfford` + `OnBalanceChanged`. Used by the game mode, second chance, shop, insufficient-coins popup and gem IAP credit.
- Fixes: soft-lock when time ran out with the ball already resting (3 s fallback, `TimeUpLandingFallbackSeconds`); combo timer now pauses during the second-chance panel.

## Refactor 3: game instance split into subsystems (Systems/)
- `USaveSubsystem`: progression + save file (async, serialized, flushed on shutdown; migrates old saves).
- `UEconomySubsystem`: coins/gems, on top of Save.
- `UStoreSubsystem`: all Play Billing (connect/reconnect, `Purchase(ProductId)`, restore remove-ads, consume gem packs), `OnAdsRemoved`.
- `UAdsSubsystem`: banner, paced interstitials, rewarded ads (`ShowRewarded` with callbacks). Pacing values stay on BP_BaseGameMode.
- `UFlappyAudioSubsystem`: background music.
- `UGameInfoInstance` now only holds BP settings (SaveGameClass, GameMusic, MaxBillingReconnectAttempts) and runs startup (music + banner).
- `IGameInstanceInterface` removed (file deleted).

## Refactor 4: ball catalog (Data/BallCatalog, Systems/BallCatalogSubsystem)
- `UBallCatalog` Data Asset: one row per ball (type, mesh, shop icon, price, owned by default). Shop order = row order.
- `UBallCatalogSubsystem`: owned/selected balls, `Unlock`, `Select`, `OnBallsChanged`. The game mode applies the selected mesh; the shop rebuilds on change.
- Save now stores only `OwnedBalls` (types) + `BallType`. Old saves (`BallsOwned`) migrate automatically (SaveVersion 0 -> 1).
- Setup: create the BallCatalog asset, set Import Save Game Class = BP_MySaveGame and Import Game Mode Class = BP_BaseGameMode, click **Import From Old Data**, save, then assign it on BP_GameInfoInstance (Shop > BallCatalog). Until then the old data is used as a fallback.
- Once the catalog is set up, `BallMeshes` (game mode) and `BallsOwned` (save defaults) are legacy and can be cleared later.

## Debug console commands (Core/DebugCommands.cpp, not in Shipping builds)
- `AddCoins 100`, `AddGems 50` (negative numbers remove), `UnlockAllBalls`, `ResetProgress` (restart after).

## Refactor 5: popups + menu/HUD split (UI/Common, UI/Menu, UI/HUD, UI/Popups)
- `UPopupWidget` base: popups are never hidden; they pop in by render scale 0 -> 1 (with overshoot) and out 1 -> 0, on real time (works while paused). Closed popups ignore taps. Tunables per popup: OpenDuration, CloseDuration, OpenOvershoot.
- Popups: Settings, Shop, InsufficientCoins, GameOver, SecondChance, new Pause.
- `UGameWidget` is now the UI root: holds MainMenuWidget, HUDWidget, PauseWidget, GameOverWidget, SecondChanceWidget, SettingsWidget, ShopWidget.
- New `UMainMenuWidget` (Play/Quit/Shop/Settings/NoAds, HighScoreText, BouncyBucketsLogo), `UHUDWidget` (PauseButton, ScoreText, ComboText, TimeProgressBar, coin/gem totals, optional ComboTextAnimation), `UPauseWidget` (ResumeButton, HomeButton, SettingsButton).
- SecondChance's old PopupShow/PopupHide animations are no longer used.

## Refactor 6: game-mode interface removed
- `IGameModeInterface` is gone; everything uses `ABaseGameMode::Get(this)` and calls the game mode directly.
- The events are public members on the game mode (`OnRunPhaseChanged`, `OnPointScored`, `OnRoundTimeUpdated`, ...), declared in `BaseGameMode.h`.
- `UFunctionsLibrary::GetGameModeInterface` removed. Delete `Public/Framework/GameModeInterface.h` (now an empty stub).

## Store prep 1+2: auto-pause and Back button
- Auto-pause: when the app loses focus or goes to the background during a run (Playing phase), the pause menu opens.
- Back (Android Back, Backspace on desktop): closes the top popup (Settings > Shop/not-enough-coins > quit confirm > Pause resumes > Game Over goes home); ignored on Second Chance; pauses during a run; on the menu opens `QuitConfirmWidget` (optional popup with YesButton/NoButton; quits directly if absent).

## Store prep 3: localized prices
- `UStoreSubsystem` queries Google Play product details for the gem packs and "remove ads" when billing connects (and when the shop registers gem products). `GetLocalizedPrice(ProductId)` returns e.g. "€0,99"; `OnPricesUpdated` fires when they arrive.
- The plugin keeps one static product-details callback, so price and purchase queries are serialized; purchases reuse the cached details (faster Play sheet).
- Gem tiles show the localized price; the Gem Shop Info `Price` is only the fallback ("$ <price>") in the editor/desktop or before Play answers.

## Store prep 4: Google consent (UMP)
- `UAdsSubsystem::StartConsentFlow()` (called at startup, before the banner): requests consent info and shows Google's consent form when required. No ad (banner, interstitial, rewarded) is requested until `UAGConsentLibrary::CanRequestAds()`; earlier banner/interstitial requests start once it's allowed.
- Settings: optional `PrivacyButton` (BindWidgetOptional) shown only when `IsPrivacyOptionsRequired()`; opens Google's privacy options form.
- Testing: `bDebugConsentAsEEA` on BP_GameInfoInstance (ignored in Shipping) + your device's ID in Project Settings > Admob Goodies > Test Device IDs.
- AdMob console: the GDPR message must be created and published (Privacy & messaging) or no form will show.

## Play Families policy fix (rejection: "Unclear ads/offers", version code 26)
- `bChildDirectedAds` (BP_GameInfoInstance, default on): tags all ad requests child-directed + under age of consent, max content rating G; consent requested as under-age (no form, non-personalized ads).
- Banner is only on screen in MainMenu and GameOver (hidden during play, second chance and pause) to avoid accidental taps.
- Main menu: optional `NoAdsPriceText` shows "Remove Ads <localized price>".
- Editor/UI to-do: label every ad/IAP entry point (see chat): "Watch Ad" text + "Ad" badge on the rewarded button, price on gem packs, "In-app purchase" wording; no ad-like buttons that look like gameplay.

## Hoop scoring fix (baskets counting from below)
- The "dropped into the rim" mark used to never expire. Now: per-ball rim entry (time + ball Z); cleared when the ball leaves RimDetector going up, touches ScoreCylinder while rising, or scores; only valid for RimToScoreMaxTime (0.75 s).
- Height check is now real: the ball's centre at rim entry must be above ScoreCylinder's centre (replaces MinimumHeightDifference, which compared fixed component positions).
- bLogScoringDecisions on the hoop logs why each ScoreCylinder touch was accepted/rejected.
