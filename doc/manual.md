
# Neverball

Tilt the floor to roll a ball through an obstacle course within the given time.

Collect coins to unlock the exit and earn extra balls.

| Coin type | Valuation |
| --------- | --------- |
| Red       | 5         |
| Blue      | 10        |
| Violet    | 25        |
| Lime      | 50        |

A ball is awarded for 100 coins, while **orange coins** worth 100 coins each.

ℹ️ ***NOTE:** If you are getting an error when trying to update then please uninstall previous version first. I'm really sorry for that, but that is the only solution for critical problem that I was able to find.*

## Setup

If the Account folder is missing in the user data directory, this will hop you straight to your setup screen. Finish the game setup in order to start.

* Language selection
* Input presets
* Terms of Services
* Player Account (Local, Online Login (optional except home edition))
* Model Selection

## Instructions

Click **Play** to begin. Mouse motion tilts the floor. Mouse buttons rotate the viewpoint. The following keyboard controls are defined by default; most of them can be changed in a configuration file. See below for details.

| Keyboard buttons | In-Game                                                               | In main menu
| ---------------- | --------------------------------------------------------------------- | --------------------------
| `ESC`            | Pause and resume                                                      | Back / Exit
| `SHIFT`          | Fast camera rotation                                                  | N/A
| `1`              | Chase Camera (Default in Neverball)                                   | N/A
| `2`              | Static Camera (Default in Switchball)                                 | N/A
| `3`              | Manual Camera                                                         | N/A
| `F9`             | Toggle frame counter                                                  | N/A
| `F1` / `F10`     | Hide HUD                                                              | =
| `F12`            | Snap a screenshot                                                     | =
| `UP`             | Tilt the floor forward                                                | Navigate up
| `DOWN`           | Tilt the floor backward                                               | Navigate down
| `LEFT`           | Tilt the floor left                                                   | Navigate left
| `RIGHT`          | Tilt the floor right                                                  | Navigate right
| `D`              | Rotate the view right                                                 | N/A
| `S`              | Rotate the view left                                                  | N/A
| `R`              | Restart the current level (only for classic, campaign and standalone) | N/A
| `TAB`            | N/A                                                                   | Cycle through scores in high-score table

**Mouse scroll wheel** zooms camera in and out.

## Campaign

Neverball levels are grouped in Campaign with Worlds in Home Edition and above. Click the left one in the level group to start the Campaign first.

In Hardcore mode is similar like an Challenge mode. All checkpoints were removed and the game cannot be saved, but with only one ball.

## Level Progression

Neverball levels are also grouped in level sets.

There are two game modes or ways of progressing through the levels: the Classic and Challenge mode.

In Classic mode, no track of lives or balls is kept. Each unlocked/completed level is immediately accessible and can be retried and restarted at any point.

In Challenge mode, the player is given a limited number of balls and attempts to complete all of the levels in turn, starting with the first level. The game ends once the balls run out and the wallet is insufficent. In addition, Google Wallet API is sent from In-Game Purchases to it's developers by the payer (available on selected Pro edition and above). A set score is recorded upon completion. Levels cannot be restarted freely.

The next set in Pro edition and below except Boost rush mode is now unlocked, once the set is completed.

A set may contain a number of bonus levels:

* **OPTION 1:** Bonus levels are unlocked by playing Challenge mode and completing all of the levels leading up to each bonus level. After unlocking a bonus level, it becomes playable in Classic mode.
* **OPTION 2:** If you're using Home edition and above, you can buy the Bonus pack in the shop, but only all bonus levels will unlock.

A set may also contain a number of master levels. After it's unlocked, the player must playing the level with Challenge mode.

## Checkpoints

Checkpoint is similar like an teleporters. You will save the spawnpoint, when you reach it. If anything bad happens, you will start over from here.

## Special Powers

This special powers is available on selected Pro edition and above.

Special powers needs help to improve the level but it gives a limited number of amounts. There are three special powers:

| Name       |                                 |
| ---------- | ------------------------------- |
| Earninator | Earn x2 coins for each level    |
| Floatifier | Reduces gravity by 50%          |
| Speedifier | Increases tilt and reduces drag |

If you want to buy more special powers, go to:

```
Main Menu > Shop.
```

## User data files

Neverball creates a directory in which it stores user data files. These files include addons, screenshots, high scores, replays, and configurations.

Under Unix and Linux, this directory is called `.neverball` (a hidden folder) and is created in the user's home directory.

Under Windows it is created in `Documents\My Games\Neverball`.

### Addons

Additional Neverball content is usually distributed as a ZIP or PK3 (a renamed ZIP) file. A properly packaged ZIP file can be installed by dropping it into the user data directory. After a restart, the game will automatically use the new content.

### Screenshots

Screenshots taken in-game with the `F12` key are stored in PNG format in the user data directory.

In Steam, it is stored only in game data directory.

### Ranks

⚠️ *This feature is under development*

ℹ️ *In-game-ranks is available on selected Home edition and above.*

In the Tradition of **Space Agency** and **SA2138**, the rank system tracks the player's progress through Campaign mode.

| Rank name        | Requires                                  | Description                                                                                                                                                                                           |
| ---------------- | ----------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Airline Cadet    | Complete Sky Level 3 to earn your wings   | This is the rank held by all trainee campaigns. Once you have completed the first three levels, you will be given your wings and awarded one of the following ranks...                                |
| Bronze Commander | Achieve all silver medals or better       | This is the lowest post-training rank. You have achieved mostly Bronze awards in your missions. You must try harder!                                                                                  |
| Silver Commander | Achieve more gold medals                  | You will be given this rank when you have achieved mostly Silver awards                                                                                                                               |
| Gold Commander   | Achieve all gold medals                   | You will be given this rank when you have achieved mostly Gold awards                                                                                                                                 |
| Elite Commander  | Be best of best!                          | Elite Commander is awarded to those extraordinary campaigns who achieve Gold in every levels they attempt. You must have flown at least 12 levels before you will be considered for the Elite status. |

### High scores

The top three fastest time through each level, the top three coin scores and the top three fastest unlock scores for each level are stored in the Scores directory within user data directory.

The top three fastest time and most coins scores for campaign are also stored. To achieve a campaign score, the player must play through all levels of campaign in Hardcore mode.

The top three fastest time and most coins scores for each level set are also stored. To achieve a set score, the player must play through all levels of a set in Challenge mode.

The total set time will include time spent during both successful and unsuccessful level plays, thus time-outs and fall-outs count against the total time.

The total campaign or set time will from checkpoint level time after respawn substracted.

### Replays

⚠️ *Some replay status limits is locked down between 16:00 - 6:00 Morning (4:00 PM - 6:00 AM Morning).*

Neverball includes a mechanism for recording and replaying levels. The player may enter a name for each replay at the end of the level. By default, the most recent unsaved level will be saved to the replay file named "Last.nbr".

Replay files are stored in the Replays directory within the user data directory. They may be copied freely. To view a replay, simply open it with the Neverball executable. You can also move it to the Replays directory and it will appear in the Replay menu in-game.

You can also restrict save and load replay filters in the Account menu, which is default by the Options menu.

Exceeded level status above the limits, which unsuccessful level plays, will also permanently deleted! In addition, [Pennyball Discord Hamburg Estates](https://discord.gg/qnJR263Hm2) scans a lots of replays and deleted it from the server, which has an Discord community server rule.

### In-Game Shop

ℹ️ *In-game-shop is available on selected Home edition and above.*

Coins are the currency with which you buy everything that is sold in the Shop. They can NOT be acquired with real money in any way, they are the games functioning in game currency like gold, coins etc. found in other games.

New balls can be purchased for 15 gems in Challenge mode, not Hardcore. To earn the gems for free, you must complete the Challenge mode in the Level Set.

If you want to buy more balls before playing Challenge mode, go to Shop.

# Configuration

ℹ️ *The game will parse the configuration text file. If it's not listed in the settings file, then the game system config will be reset by default.*

Game settings are stored in the file neverballrc in the user data directory. This file is created when it changes from in-game settings or the game exits. It consists of key/value pairs. Other meaningful keys and their default values follow.

## Online Services Settings

```
dedicated_ipaddress pennyball.stynegame.de
dedicated_ipport    5000
```

This key specifies the IPv4/IPv6 address or host and port of your dedicated server. For more information, see the section on using dedicated server with Neverball.

## Account Settings

```
ball_file ball/basic-ball/basic-ball
```

This key determines the model used for the ball. Changing the player name will your model automatically replaced.

```
replay_name %s-%l-%r
```

⚠️ *The resulting replay name is also suffixed by an underscore and a unique date format to avoid name collisions with existing replays.*

This key specifies the format of the default replay name generated when saving replays.

The value of replay_name can include regular characters and special character sequences which act as place-holders for certain "dynamic" text. These sequences are recognised:

| Format |                                                  |
| ------ | ------------------------------------------------ |
| %s     | current set identifier (such as "easy" or "mym") |
| %l     | current level identifier (such as "03" or "IV")  |
| %r     | current level status (such as "xf" or "xt")      |
| %%     | single percentage sign                           |

Any other sequence starting with % is ignored.

```
screenshot 0
```

This key holds the current screenshot index. The number is incremented every time a new screenshot is taken (by pressing F12) and it is appended to the image file name. In Steam you cannot set this value.

## Display Settings

```
width  1200
height 900
```

These keys determine the effective display resolution. If for any reason the resolution you're looking for isn't available in the in-game settings, you can modify these values instead.

```
fullscreen 0
```

This key determines whether or not the application starts full screen.

```
display 0
```

Selects on which display the game window is placed.

## Control Settings

### Gamepad Settings

```
joystick 1
```

This key enables joystick control. `0` is off, `1` is on. The game may still be controlled with the mouse even while gamepad control is enabled. However, random noise from an analog controller at rest can disrupt normal mouse input.

```
joystick_axis_x 0
```

Joystick horizontal axis number

```
joystick_axis_y 1
```

Joystick vertical axis number

```
joystick_axis_u 2
```

Joystick axis number for view rotation control

```
joystick_button_a 0
```

Joystick menu select button

```
joystick_button_b 1
```

Joystick menu cancel button
```
joystick_button_r 2
```

Joystick counter-clockwise camera rotation button

```
joystick_button_l 3
```

Joystick clockwise camera rotation button

```
joystick_button_exit 4
```

Joystick exit button

```
wiimote_addr
```

This key specifies the address of your Nintendo Wii Remote. For more information, see the section on using Wiimote with Neverball.

### Tilting Floor

```
tilting_floor 1
```

This key controls neither tilt the floor nor move the ball. If you're using Campaign, you cannot enable this option. (Available for selected version 1.9 and above)

### Mouse Sensitivity

```
mouse_sense 300
```

This key controls mouse sensitivity. The value gives the number of screen pixels the mouse pointer must move to rotate the floor through its entire range. A smaller number means more sensitive.

### Invert Mouse Y

```
mouse_invert 1
```

This key inverts the vertical mouse axis if set to 1.

### Camera key bindings

```
key_camera_1      1
key_camera_2      2
key_camera_3      3
key_camera_l      s
key_camera_r      d
key_camera_toggle e
```

These keys define keyboard mappings for camera selection and rotation. The three camera behaviors are as follows:

| Keyboard buttons |                                                                                                                               |
| ---------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `1`              | Chase camera stays behind the ball by cueing off of the velocity of the ball. It is very responsive, but sometimes confusing. |
| `2`              | Static camera stays at the specific camera position.                                                                          |
| `3`              | Manual camera does not rotate except by  player command.                                                                      |

The default values for these keys changed with version 1.7.1. Some players may be interested in using the new values. They were as follows:

```
key_camera_1      3
key_camera_2      1
key_camera_3      2
key_camera_l      a
key_camera_r      d
key_camera_toggle c
```

`key_camera_toggle` toggles camera behaviour between 1 and 3.

### Camera mouse bindings

```
mouse_camera_1      none
mouse_camera_2      none
mouse_camera_3      none
mouse_camera_l      left
mouse_camera_r      right
mouse_camera_toggle middle
```

These keys match  the respective key_camera_* options.

Accepted values are: "none" (for no mapping), "left", "right", "wheelup", "middle", "wheeldown" or a numeric mouse button index.

### Camera rotation settings

```
rotate_fast 300
rotate_slow 150
```

These keys control the rate of camera rotation. Roughly, they give the rate of lateral camera motion in centimeters per seconds, so the actual rotation rate depends upon view_dz, above. The fast rate is used when the Shift key is held down.

```
touch_rotate 16
```

⚠️ *This control settings is only for touchscreen*

Defines the fraction of the screen that you need to swipe across to reach rotate_slow rotation speed. E.g., a value of 16 means 1/16 of screen. You can swipe farther to reach the rotate_fast rotation speed.

### Invert camera rotation rate

```
camera_rotate_mode 1
```

This key changes camera rotation mode. There are as follows:

| Value |                                                                                                                                                       |
| ----- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| `0`   | When "Inverted" is set, select this option will switch the camera rotation rate exactly. Useful when playing this level using Switchball HD controls. |
| `1`   | Flip camera rotation (default).                                                                                                                       |

### Tilt controls

```
key_forward  up
key_backward down
key_left     left
key_right    right
```

These keys define keyboard mappings for tilt control.

### Restart Level

```
key_restart r
```

This key defines a keyboard mapping for a mid-game restart of the current level. Handy when trying to record a new high-score, this function isn't available in challenge mode.

### Cycle highscore tabs

```
key_score_next tab
```

This key defines a keyboard mapping for cycling through Most Coins / Best Times / Fast Unlock score tabs in the high-score board.

## Camera settings

### Camera position settings

```
view_fov 50
view_dp  75
view_dc  25
view_dz  200
```

⚠️ *Height, center and horizontal distance was removed after 2.2 and is superceded with Zoom cameras using Switchball Classic, XBLA and HD.*

The default values for these keys changed with version 1.2.6. Some players may be interested in using the old values. They were as follows:

```
view_fov 40
view_dp  400
view_dc  0
view_dz  600
```

These keys define the view of the ball. They give the field of view in degrees, the height of the view point, the height of the view center, and the horizontal distance from the ball in centimeters, respectively. (The ball is 50 centimeters in diameter in most levels.)

## UI Settings

```
screen_animation 1
```

This key enables the menu screen animations.

## Performance Settings

```
fps 0
```

This key enables an on-screen frames-per-second counter. Press F9 to toggle this flag in-game.

```
nice 0
```

This key enables a delay function after each frame is rendered, forcing a context switch and ensuring that the game does not utilize 100% of the CPU. 0 is off, 1 is on.

If the frame rate is not fast enough for you, or you simply want to test the performance of the game on your hardware, disable it.

See config keys "smooth_fix" and "force_smooth_fix" for more information.

```
stats 0
```

This key enables print-out (to standard output) of running statistics of the current frame time and frames-per-second, averaged over one second. In Steam version or version 1.7+, you cannot enable this option.

```
stereo 0
```

This key enables quad-buffered stereo viewing for those with the hardware to support it. 1 is on, 0 is off.

```
smooth_fix 1
```

This key optimizes the game, that can reduce lag. Disable it if game speed becomes inconsistent.

```
force_smooth_fix 0
```

Smooth fix is normally disabled, if a level is lagging, this forces smooth fix to remain enabled. Toggle to test if the performance is better with smooth fix always enabled.

```
vsync 1
```

This key controls vertical blanking synchronization. `1` is on (and is the default), `0` is off.

```
multisample 0
```

This key enables multisample full-screen antialiasing. Values can be `2`, `4`, `8`, etc., and can be overspecified; in such case the game will search for the highest level of multisampling supported by your hardware. (The best value eventually gets written to the config file.)
```
mipmap 1
aniso  0
```

These keys control mipmapping and anisotropic filtering, respectively.

With mipmapping, smaller versions of each texture are kept in video memory, and are referenced when a texture is viewed from a distance. This improves video cache coherence and eliminates texture "swimming" on detailed textures when seen from afar.

- *To disable mipmapping, set mipmap to `0`.*

Related to mipmapping is anisotropic filtering. "Anisotropic" basically means "not the same from all directions". It refers to cases where a texture might need to be compressed more vertically than horizontally. For example, if a texture is applied to a flat surface and seen from far away then it appears much wider than high. Anisotropic filtering takes care of this. Its level is expressed as a small power of two.

- *To disable anisotropic, set aniso to `0`.*

🌐 *Web: https://neverball.org/*
