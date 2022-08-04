*******************************************
* BotAim 2 - human-like bot aiming Plugin *
*******************************************
by Pierre-Marie Baty <pm@bots-united.com>


This metamod plugin allows bot server administrators to change the way their
bots turn and aim in any Half-Life MOD to a more realistic method.
Technically, any bot that use the "idealpitch" and "ideal_yaw" field of its
entity variables structure for storing the final aim angles can be affected by
this plugin. Since most bot authors used botman's template, many bots are ok.

PLEASE NOTE that this plugin does not change the bot's AI, it does not alter
their decisions and ESPECIALLY IT DOES NOT CHANGE WHERE THEY WANT TO LOOK. It
just enables them to move the crosshair in a smoothened way. If you see your
bots aiming at stupid locations, or trying to look through walls, blame the
bot, NOT the plugin!

The algorithm implemented assimilates the crosshair movement to a spring with
several physical constraints. Since the generic equation describing a spring's
movement is well-known by modern physics, this modelization enables the user
to define the bot aiming's parameters as the stiffness and damping coefficient
of the equivalent spring.

The aiming system is HIGHLY configurable. You can set it to suit your tastes
in all domains. The aiming can be greatly affected by your changes, and not
two servers using this plugin will feel the same if the settings are not
identical. You are warmly encouraged to toy with the CVAR's values to find
the perfect aiming that will challenge you ;-)


featured CVARS:

botaim_enable (default: 1)
   Enables or disables the plugin. Set to 1 (enabled) by default. Set to 0 to
   disable BotAim and restore the bot's original aiming algorithms.

botaim_spring_stiffness_x (default: 13.0)
   VERTICAL (up/down) spring stiffness of the aiming system. This value affects
   the quickness of the crosshair movement, as well as the frequency of the
   resulting oscillations.

botaim_spring_stiffness_y (default: 13.0)
   HORIZONTAL (left/right) spring stiffness of the aiming system. This value
   affects the quickness of the crosshair movement, as well as the frequency
   of the resulting oscillations.

botaim_damper_coefficient_x (default: 2.0)
   VERTICAL (up/down) damping coefficient of the aiming system. This value
   also affects the quickness of the crosshair movement, and the amplitude of
   the resulting oscillations.

botaim_damper_coefficient_y (default: 2.0)
   HORIZONTAL (left/right) damping coefficient of the aiming system. This value
   also affects the quickness of the crosshair movement, and the amplitude of
   the resulting oscillations.

botaim_deviation_x (default: 20.0)
   VERTICAL (up/down) error margin of the aiming system. This value affects how
   far (in game length units) the bot will allow its crosshair to derivate from
   the ideal direction when not targeting any player in particular.

botaim_deviation_y (default: 90.0)
   HORIZONTAL (left/right) error margin of the aiming system. This value
   affects how far (in game length units) the bot will allow its crosshair to
   derivate from the ideal direction when not targeting any player in
   particular.

botaim_influence_x_on_y (default: 0.25)
   Perpendicular influence of the vertical axis on a HORIZONTAL movement. This
   value affects how much (in fraction of 1) the bot will be disturbed when
   moving its crosshair on an axis by the inherent movement on the other axis.

botaim_influence_y_on_x (default: 0.17)
   Perpendicular influence of the horizontal axis on a VERTICAL movement. This
   value affects how much (in fraction of 1) the bot will be disturbed when
   moving its crosshair on an axis by the inherent movement on the other axis.

botaim_offset_delay (default: 1.2)
   Maximal value in seconds after which the aiming system will re-evaluate its
   ideal direction. This affects how often a bot will attempt to correct an
   imprecise crosshair placement by moving the crosshair inside the error
   margin bounds defined by the botaim_deviation CVARs.

botaim_notarget_slowdown_ratio (default: 0.3)
   Fraction of the full speed the aiming system will adopt as speed when aiming
   at nothing in particular. This affects how slow the bot will move its
   crosshair when targeting nobody, relatively to its full speed capacity.

botaim_target_anticipation_ratio (default: 0.8)
   Fraction of the estimated target's velocity the aiming system will rely on
   when aiming at a moving target. This affects how well the bot will be able
   to track moving targets on the fly, and whether it will have a tendancy to
   aim ahead of it or behind it.

botaim_fix (default: 1)
   This CVAR tells whether the vertical (x) component of the bot's body angles
   should be negated, due to a very common bug in Half-Life bots that was
   already present in botman's template, causing these angles to be reverted.
   If you see the bots looking up when they should look down and inversely
   looking down when they should look up, try setting this CVAR to its opposite
   value. The default is 1 (aimbug fix activated).


Long live bot servers!
