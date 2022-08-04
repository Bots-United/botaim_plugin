// ********************************
// * Human-like Bot Aiming Plugin *
// ********************************
// by Pierre-Marie Baty <pm@bots-united.com>


#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>


typedef struct
{
   edict_t *pEdict;
   edict_t *pTarget;
   Vector v_origin;
   Vector v_eyeorigin;
   Vector targeted_location;
   Vector v_angle;
   Vector ideal_angles;
   Vector v_randomization;
   Vector randomized_ideal_angles;
   Vector angular_deviation;
   Vector aim_speed;
   Vector target_angular_speed;
   float randomize_angles_time;
   float player_target_time;
   float last_shoot_time;
} bot_t;


enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;
DLL_FUNCTIONS gFunctionTable_Post;
cvar_t botaim_enable = {"botaim_enable", "1", FCVAR_EXTDLL};
cvar_t botaim_spring_stiffness_x = {"botaim_spring_stiffness_x", "13.0", FCVAR_EXTDLL};
cvar_t botaim_spring_stiffness_y = {"botaim_spring_stiffness_y", "13.0", FCVAR_EXTDLL};
cvar_t botaim_damper_coefficient_x = {"botaim_damper_coefficient_x", "0.22", FCVAR_EXTDLL};
cvar_t botaim_damper_coefficient_y = {"botaim_damper_coefficient_y", "0.22", FCVAR_EXTDLL};
cvar_t botaim_deviation_x = {"botaim_deviation_x", "20.0", FCVAR_EXTDLL};
cvar_t botaim_deviation_y = {"botaim_deviation_y", "90.0", FCVAR_EXTDLL};
cvar_t botaim_influence_x_on_y = {"botaim_influence_x_on_y", "0.25", FCVAR_EXTDLL};
cvar_t botaim_influence_y_on_x = {"botaim_influence_y_on_x", "0.17", FCVAR_EXTDLL};
cvar_t botaim_offset_delay = {"botaim_offset_delay", "1.2", FCVAR_EXTDLL};
cvar_t botaim_notarget_slowdown_ratio = {"botaim_notarget_slowdown_ratio", "0.3", FCVAR_EXTDLL};
cvar_t botaim_target_anticipation_ratio = {"botaim_target_anticipation_ratio", "0.8", FCVAR_EXTDLL};
cvar_t botaim_fix = {"botaim_fix", "1", FCVAR_EXTDLL};
cvar_t botaim_debug = {"botaim_debug", "0", FCVAR_EXTDLL};
bool botaim_enabled;
Vector spring_stiffness;
Vector damper_coefficient;
Vector influence;
Vector randomization;
float offset_delay;
float notarget_slowdown_ratio;
float target_anticipation_ratio;
bool aim_fix;
bool aim_debug;
bot_t bots[32];
int beam_texture;
edict_t *pListenserverEdict;
const Vector g_vecZero = Vector (0, 0, 0);

void WINAPI GiveFnptrsToDll (enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals);
int Spawn_Post (edict_t *pent);
void StartFrame_Post ();
void TraceLine (const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);
void BotPointGun (bot_t *pBot, bool save_angles);
Vector UTIL_VecToAngles (const Vector &v_vector);
float UTIL_AngleOfVectors (Vector v1, Vector v2);
float UTIL_WrapAngle (float angle);
Vector UTIL_WrapAngles (Vector angles);
void UTIL_DrawBeam (Vector start, Vector end, int red, int green, int blue);


// START of Metamod stuff

enginefuncs_t meta_engfuncs;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;
meta_globals_t *gpMetaGlobals;

META_FUNCTIONS gMetaFunctionTable =
{
	nullptr, // pfnGetEntityAPI()
	nullptr, // pfnGetEntityAPI_Post()
	nullptr, // pfnGetEntityAPI2()
   GetEntityAPI2_Post, // pfnGetEntityAPI2_Post()
	nullptr, // pfnGetNewDLLFunctions()
	nullptr, // pfnGetNewDLLFunctions_Post()
   GetEngineFunctions, // pfnGetEngineFunctions()
	nullptr, // pfnGetEngineFunctions_Post()
};

plugin_info_t Plugin_info = {
   META_INTERFACE_VERSION, // interface version
   "BotAim", // plugin name
   "2.4-APG", // plugin version
   "03/08/2022", // date of creation
   "Pierre-Marie Baty <pm@bots-united.com>", // plugin author
   "http://racc.bots-united.com", // plugin URL
   "BOTAIM", // plugin logtag
   PT_ANYTIME, // when loadable
   PT_ANYTIME, // when unloadable
};


C_DLLEXPORT int Meta_Query (char *ifvers, plugin_info_t **pPlugInfo, mutil_funcs_t *pMetaUtilFuncs)
{
   // this function is the first function ever called by metamod in the plugin DLL. Its purpose
   // is for metamod to retrieve basic information about the plugin, such as its meta-interface
   // version, for ensuring compatibility with the current version of the running metamod.

   // keep track of the pointers to metamod function tables metamod gives us
   gpMetaUtilFuncs = pMetaUtilFuncs;
   *pPlugInfo = &Plugin_info;

   // check for interface version compatibility
   if (strcmp (ifvers, Plugin_info.ifvers) != 0)
   {
      int mmajor = 0, mminor = 0, pmajor = 0, pminor = 0;

      LOG_CONSOLE (PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);
      LOG_MESSAGE (PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);

      // if plugin has later interface version, it's incompatible (update metamod)
      sscanf (ifvers, "%d:%d", &mmajor, &mminor);
      sscanf (META_INTERFACE_VERSION, "%d:%d", &pmajor, &pminor);

      if ((pmajor > mmajor) || ((pmajor == mmajor) && (pminor > mminor)))
      {
         LOG_CONSOLE (PLID, "metamod version is too old for this plugin; update metamod");
         LOG_ERROR (PLID, "metamod version is too old for this plugin; update metamod");
         return (false);
      }

      // if plugin has older major interface version, it's incompatible (update plugin)
      else if (pmajor < mmajor)
      {
         LOG_CONSOLE (PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");
         LOG_ERROR (PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");
         return (false);
      }
   }

   return (true); // tell metamod this plugin looks safe
}


C_DLLEXPORT int Meta_Attach (PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs)
{
   // this function is called when metamod attempts to load the plugin. Since it's the place
   // where we can tell if the plugin will be allowed to run or not, we wait until here to make
   // our initialization stuff, like registering CVARs and dedicated server commands.

   // are we allowed to load this plugin now ?
   if (now > Plugin_info.loadable)
   {
      LOG_CONSOLE (PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);
      LOG_ERROR (PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);
      return (false); // returning false prevents metamod from attaching this plugin
   }

   // keep track of the pointers to engine function tables metamod gives us
   gpMetaGlobals = pMGlobals;
   memcpy (pFunctionTable, &gMetaFunctionTable, sizeof (META_FUNCTIONS));
   gpGamedllFuncs = pGamedllFuncs;

   // print a message to notify about plugin attaching
   LOG_CONSOLE (PLID, "%s: plugin attaching", Plugin_info.name);
   LOG_MESSAGE (PLID, "%s: plugin attaching", Plugin_info.name);

   // ask the engine to register the CVARs this plugin uses
   CVAR_REGISTER (&botaim_enable);
   CVAR_REGISTER (&botaim_spring_stiffness_x);
   CVAR_REGISTER (&botaim_spring_stiffness_y);
   CVAR_REGISTER (&botaim_damper_coefficient_x);
   CVAR_REGISTER (&botaim_damper_coefficient_y);
   CVAR_REGISTER (&botaim_deviation_x);
   CVAR_REGISTER (&botaim_deviation_y);
   CVAR_REGISTER (&botaim_influence_x_on_y);
   CVAR_REGISTER (&botaim_influence_y_on_x);
   CVAR_REGISTER (&botaim_offset_delay);
   CVAR_REGISTER (&botaim_notarget_slowdown_ratio);
   CVAR_REGISTER (&botaim_target_anticipation_ratio);
   CVAR_REGISTER (&botaim_fix);
   CVAR_REGISTER (&botaim_debug);

   return (true); // returning true enables metamod to attach this plugin
}


C_DLLEXPORT int Meta_Detach (PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
   // this function is called when metamod unloads the plugin. A basic check is made in order
   // to prevent unloading the plugin if its processing should not be interrupted.

   // is metamod allowed to unload the plugin ?
   if ((now > Plugin_info.unloadable) && (reason != PNL_CMD_FORCED))
   {
      LOG_CONSOLE (PLID, "%s: plugin NOT detaching (can't unload plugin right now)", Plugin_info.name);
      LOG_ERROR (PLID, "%s: plugin NOT detaching (can't unload plugin right now)", Plugin_info.name);
      return (false); // returning false prevents metamod from unloading this plugin
   }

   return (true); // returning true enables metamod to unload this plugin
}


C_DLLEXPORT int GetEntityAPI2_Post (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
   gFunctionTable_Post.pfnSpawn = Spawn_Post;
   gFunctionTable_Post.pfnStartFrame = StartFrame_Post;

   memcpy (pFunctionTable, &gFunctionTable_Post, sizeof (DLL_FUNCTIONS));
   return (true);
}


C_DLLEXPORT int GetEngineFunctions (enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{
   meta_engfuncs.pfnTraceLine = TraceLine;

   memcpy (pengfuncsFromEngine, &meta_engfuncs, sizeof (enginefuncs_t));
   return (true);
}

// END of Metamod stuff




#ifdef _WIN32
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   return (true); // required Win32 DLL entry point
}
#endif


void WINAPI GiveFnptrsToDll (enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals)
{
   memcpy (&g_engfuncs, pengfuncsFromEngine, sizeof (enginefuncs_t));
   gpGlobals = pGlobals;
}


int Spawn_Post (edict_t *pent)
{
   // if we are spawning the world itself (worldspawn is the first entity spawned)
   if (strcmp (STRING (pent->v.classname), "worldspawn") == 0)
      beam_texture = PRECACHE_MODEL ("sprites/lgtning.spr"); // used to trace beams

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


void StartFrame_Post ()
{
   edict_t *pPlayer;
   bot_t *pBot;
   int player_index;

   botaim_enabled = (CVAR_GET_FLOAT ("botaim_enable") > 0);

   // is the plugin disabled ?
   if (!botaim_enabled)
      RETURN_META (MRES_IGNORED); // then don't do anything

   // get the CVARs
   spring_stiffness.x = CVAR_GET_FLOAT ("botaim_spring_stiffness_x");
   spring_stiffness.y = CVAR_GET_FLOAT ("botaim_spring_stiffness_y");
   damper_coefficient.x = CVAR_GET_FLOAT ("botaim_damper_coefficient_x");
   damper_coefficient.y = CVAR_GET_FLOAT ("botaim_damper_coefficient_y");
   randomization.x = CVAR_GET_FLOAT ("botaim_deviation_x");
   randomization.y = CVAR_GET_FLOAT ("botaim_deviation_y");
   influence.x = CVAR_GET_FLOAT ("botaim_influence_x_on_y");
   influence.y = CVAR_GET_FLOAT ("botaim_influence_y_on_x");
   offset_delay = CVAR_GET_FLOAT ("botaim_offset_delay");
   notarget_slowdown_ratio = CVAR_GET_FLOAT ("botaim_notarget_slowdown_ratio");
   target_anticipation_ratio = CVAR_GET_FLOAT ("botaim_target_anticipation_ratio");
   aim_fix = (CVAR_GET_FLOAT ("botaim_fix") > 0);
   aim_debug = (CVAR_GET_FLOAT ("botaim_debug") > 0);

   // cycle through all players
   for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
   {
      pPlayer = INDEXENT (player_index); // quick access to player
      pBot = &bots[player_index - 1]; // quick access to corresponding bot

      if (FNullEnt (pPlayer))
         continue; // skip invalid players

      // has this player/bot just respawned (check the origins) ?
      if ((pPlayer->v.origin - pBot->v_origin).Length () > 200)
         memset (pBot, 0, sizeof (bot_t)); // if so, reset botaim

      pBot->pEdict = pPlayer; // save bot's edict
      pBot->v_origin = pPlayer->v.origin; // save bot's origin
      pBot->v_eyeorigin = pPlayer->v.origin + pPlayer->v.view_ofs; // save bot's eye position

      // systematically wrap all angles to avoid engine freezes
      pBot->v_angle = UTIL_WrapAngles (pBot->v_angle);
      pBot->ideal_angles = UTIL_WrapAngles (pBot->ideal_angles);
      pBot->randomized_ideal_angles = UTIL_WrapAngles (pBot->randomized_ideal_angles);
      pBot->angular_deviation = UTIL_WrapAngles (pBot->angular_deviation);
      pBot->aim_speed = UTIL_WrapAngles (pBot->aim_speed);
      pBot->target_angular_speed = UTIL_WrapAngles (pBot->target_angular_speed);
      pPlayer->v.angles = UTIL_WrapAngles (pPlayer->v.angles);
      pPlayer->v.v_angle = UTIL_WrapAngles (pPlayer->v.v_angle);
      pPlayer->v.idealpitch = UTIL_WrapAngle (pPlayer->v.idealpitch);
      pPlayer->v.ideal_yaw = UTIL_WrapAngle (pPlayer->v.ideal_yaw);

      // if this player is a bot AND it is alive, make it aim and turn
      if ((pPlayer->v.flags & FL_FAKECLIENT) && (pPlayer->v.deadflag != DEAD_DEAD))
         BotPointGun (pBot, true); // update and save this bot's view angles

      // else keep track of the first human we find as the listen server entity
      else if (FNullEnt (pListenserverEdict))
         pListenserverEdict = pPlayer;

      // systematically wrap all angles again to avoid engine freezes
      pBot->v_angle = UTIL_WrapAngles (pBot->v_angle);
      pBot->ideal_angles = UTIL_WrapAngles (pBot->ideal_angles);
      pBot->randomized_ideal_angles = UTIL_WrapAngles (pBot->randomized_ideal_angles);
      pBot->angular_deviation = UTIL_WrapAngles (pBot->angular_deviation);
      pBot->aim_speed = UTIL_WrapAngles (pBot->aim_speed);
      pBot->target_angular_speed = UTIL_WrapAngles (pBot->target_angular_speed);
      pPlayer->v.angles = UTIL_WrapAngles (pPlayer->v.angles);
      pPlayer->v.v_angle = UTIL_WrapAngles (pPlayer->v.v_angle);
      pPlayer->v.idealpitch = UTIL_WrapAngle (pPlayer->v.idealpitch);
      pPlayer->v.ideal_yaw = UTIL_WrapAngle (pPlayer->v.ideal_yaw);
   }

   RETURN_META (MRES_IGNORED);
}


void TraceLine (const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
   // check to see if a bot is sending this traceline AND it is firing AND we are taking over the bot aiming code
   if (botaim_enabled && !FNullEnt (pentToSkip) && (pentToSkip->v.flags & FL_FAKECLIENT)
       && (pentToSkip->v.button & (IN_ATTACK | IN_ATTACK2)))
   {
      BotPointGun (&bots[ENTINDEX (pentToSkip) - 1], false); // update bot's view angles in advance, but don't save them
      MAKE_VECTORS (pentToSkip->v.v_angle); // build base vectors in bot's actual view direction

      // trace line considering the ACTUAL view angles of the bot, not the ones it claims to have
      TRACE_LINE (pentToSkip->v.origin + pentToSkip->v.view_ofs,
                  pentToSkip->v.origin + pentToSkip->v.view_ofs + gpGlobals->v_forward * 9999,
                  fNoMonsters, pentToSkip, ptr);

      RETURN_META (MRES_SUPERCEDE); // and don't let the engine be fooled by the bot's lies, goddamnit! :)
   }

   RETURN_META (MRES_IGNORED);
}


void BotPointGun (bot_t *pBot, bool save_angles)
{
   // this function is called every frame for every bot. Its purpose is to make the bot
   // move its crosshair to the direction where it wants to look. There is some kind of
   // filtering for the view, to make it human-like.

   TraceResult tr;
   Vector v_stiffness;
   float stiffness_multiplier;
   edict_t *pPlayer;
   int player_index;
   Vector angle_diff;

   // build the bot's ideal angles
   if (aim_fix)
      pBot->ideal_angles = Vector (-pBot->pEdict->v.idealpitch, pBot->pEdict->v.ideal_yaw, 0);
   else
      pBot->ideal_angles = Vector (pBot->pEdict->v.idealpitch, pBot->pEdict->v.ideal_yaw, 0);

   // find out if the bot is aiming at something
   MAKE_VECTORS (pBot->ideal_angles);
   TRACE_LINE (pBot->v_eyeorigin, pBot->v_eyeorigin + gpGlobals->v_forward * 9999,
               dont_ignore_monsters, pBot->pEdict, &tr);

   // is bot aiming at a player (or another bot) OR a breakable (thanks Splo) ?
   if ((tr.pHit->v.flags & (FL_CLIENT | FL_FAKECLIENT))
       || (strcmp (STRING (tr.pHit->v.classname), "func_breakable") == 0))
   {
      pBot->pTarget = tr.pHit; // save bot's targeted player
      pBot->player_target_time = gpGlobals->time;
   }
   else
   {
      // 2nd chance, try seeing if we're aiming right near another player
      for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
      {
         pPlayer = INDEXENT (player_index); // quick access to player

         if (FNullEnt (pPlayer) || (pPlayer == pBot->pEdict))
            continue; // skip invalid players and skip self

         // see if this player is visible
         TRACE_LINE (pBot->v_eyeorigin, pPlayer->v.origin, ignore_monsters, pBot->pEdict, &tr);

         // is this player visible AND are we less than 5 degrees on him ?
         if ((tr.flFraction == 1.0) && (UTIL_WrapAngles (UTIL_VecToAngles (pPlayer->v.origin - pBot->v_origin)
                                                         - pBot->ideal_angles).Length () < 5))
         {
            pBot->pTarget = pPlayer; // if so, assume he's our target
            pBot->player_target_time = gpGlobals->time;
            break; // and stop searching
         }
      }

      // have we found no candidate ?
      if (player_index > gpGlobals->maxClients)
         pBot->pTarget = nullptr; // bot is aiming at nothing in particular
   }

   // save bot's targeted location
   pBot->targeted_location = tr.vecEndPos;

   // save bot's last shooting time
   if (pBot->pEdict->v.button & (IN_ATTACK | IN_ATTACK2))
      pBot->last_shoot_time = gpGlobals->time;

   // is bot aiming at something OR was bot aiming at something very recently ?
   if (pBot->player_target_time + offset_delay > gpGlobals->time)
   {
      // bots usually tend to aim a bit too high, it can't hurt forcing them down a bit
      pBot->randomized_ideal_angles = UTIL_VecToAngles (pBot->targeted_location + Vector (0, 0, -6)
                                                        - pBot->v_eyeorigin);

      // get the target's angular speed (if appliable)
      if (!FNullEnt (pBot->pTarget))
         pBot->target_angular_speed = UTIL_WrapAngles ((UTIL_VecToAngles (pBot->pTarget->v.origin - pBot->v_origin + pBot->pTarget->v.velocity - pBot->pEdict->v.velocity)
                                                        - UTIL_VecToAngles (pBot->pTarget->v.origin - pBot->v_origin))
                                                       * target_anticipation_ratio);

      // set the overall stiffness for a fast aim
      v_stiffness = spring_stiffness;
   }

   // else bot is aiming at nothing in particular
   else
   {
      // is it time for bot to randomize the aim direction again (more often where moving) ?
      if ((pBot->randomize_angles_time < gpGlobals->time)
          && (((pBot->pEdict->v.velocity.Length () > 1.0) && (pBot->angular_deviation.Length () < 5.0))
              || (pBot->angular_deviation.Length () < 1.0)))
      {
         // is the bot standing still ? (thanks sPlOrYgOn again for the bug fix)
         if (pBot->pEdict->v.velocity.Length () < 1.0)
            pBot->v_randomization = randomization * 0.2; // randomize less than normal
         else
            pBot->v_randomization = randomization; // normal randomization

         // randomize targeted location a bit (slightly towards the ground)
         pBot->randomized_ideal_angles = UTIL_VecToAngles (pBot->targeted_location
                                                           + Vector (RANDOM_FLOAT (-pBot->v_randomization.y, pBot->v_randomization.y),
                                                                     RANDOM_FLOAT (-pBot->v_randomization.y, pBot->v_randomization.y),
                                                                     RANDOM_FLOAT (-pBot->v_randomization.x * 1.5, pBot->v_randomization.x * 0.5))
                                                           - (pBot->v_eyeorigin));

         // set next time to do this
         pBot->randomize_angles_time = gpGlobals->time + RANDOM_FLOAT (0.4, offset_delay);
      }

      // no target means no angular speed to take in account
      pBot->target_angular_speed = g_vecZero;

      stiffness_multiplier = notarget_slowdown_ratio; // slow aim by default

      // take in account whether the bot was targeting someone in the last N seconds
      if (gpGlobals->time - (pBot->player_target_time + offset_delay) < notarget_slowdown_ratio * 10.0)
         stiffness_multiplier = 1 - (gpGlobals->time - pBot->last_shoot_time) * 0.1;

      // also take in account the remaining deviation (slow down the aiming in the last 10°)
      if (pBot->angular_deviation.Length () < 10.0)
         stiffness_multiplier *= pBot->angular_deviation.Length () * 0.1;

      // slow down even more if we are not moving
      if (pBot->pEdict->v.velocity.Length () < 1)
         stiffness_multiplier *= 0.5;

      // but don't allow getting below a certain value
      if (stiffness_multiplier < 0.2)
         stiffness_multiplier = 0.2;

      v_stiffness = spring_stiffness * stiffness_multiplier; // increasingly slow aim
   }

   // compute randomized angle deviation this time
   pBot->angular_deviation = UTIL_WrapAngles (pBot->randomized_ideal_angles - pBot->v_angle);

   // spring/damper model aiming (thanks Aspirin for the target speed idea)
   pBot->aim_speed.x = (v_stiffness.x * pBot->angular_deviation.x) - (damper_coefficient.x * (pBot->aim_speed.x - pBot->target_angular_speed.x));
   pBot->aim_speed.y = (v_stiffness.y * pBot->angular_deviation.y) - (damper_coefficient.y * (pBot->aim_speed.y - pBot->target_angular_speed.y));

   // influence of y movement on x axis and vice versa (less influence than x on y since it's
   // easier and more natural for the bot to "move its mouse" horizontally than vertically)
   pBot->aim_speed.x += pBot->aim_speed.y * influence.y;
   pBot->aim_speed.y += pBot->aim_speed.x * influence.x;

   // move the aim cursor
   pBot->pEdict->v.v_angle = UTIL_WrapAngles (pBot->v_angle + gpGlobals->frametime * Vector (pBot->aim_speed.x, pBot->aim_speed.y, 0));

   // set the body angles to point the gun correctly
   pBot->pEdict->v.angles.x = -pBot->pEdict->v.v_angle.x / 3;
   pBot->pEdict->v.angles.y = pBot->pEdict->v.v_angle.y;
   pBot->pEdict->v.angles.z = 0;

   // if we want to save the bot's view angles, remember these view angles
   if (save_angles)
      pBot->v_angle = pBot->pEdict->v.v_angle;

   // if we are in debug mode and this bot is being spectated, display the beams
   if (aim_debug && (pListenserverEdict->v.origin - pBot->v_origin).Length () < 10.0)
   {
      MAKE_VECTORS (Vector (pBot->pEdict->v.idealpitch, pBot->pEdict->v.ideal_yaw, 0));
      UTIL_DrawBeam (pBot->v_eyeorigin, pBot->v_eyeorigin + gpGlobals->v_forward * 300, 255, 255, 255);

      MAKE_VECTORS (pBot->randomized_ideal_angles);
      UTIL_DrawBeam (pBot->v_eyeorigin, pBot->v_eyeorigin + gpGlobals->v_forward * 300, 255, 0, 0);

      MAKE_VECTORS (pBot->v_angle);
      UTIL_DrawBeam (pBot->v_eyeorigin, pBot->v_eyeorigin + gpGlobals->v_forward * 300, 0, 0, 255);
   }

   return;
}


Vector UTIL_VecToAngles (const Vector &v_vector)
{
   float angles[3];

   VEC_TO_ANGLES (v_vector, angles);

   return (UTIL_WrapAngles (Vector (angles)));
}


float UTIL_AngleOfVectors (Vector v1, Vector v2)
{
   if ((v1.Length () == 0) || (v2.Length () == 0))
      return (0); // avoid zero divide

   return (UTIL_WrapAngle (acos (DotProduct (v1, v2) / (v1.Length () * v2.Length ())) * 180 / M_PI));
}


float UTIL_WrapAngle (float angle)
{
   // check for wraparound of angle
   if (angle >= 180)
      angle -= 360 * ((int) (angle / 360) + 1);
   if (angle < -180)
      angle += 360 * ((int) (angle / 360) + 1);

   return (angle);
}


Vector UTIL_WrapAngles (Vector angles)
{
   // check for wraparound of angles
   if (angles.x >= 180)
      angles.x -= 360 * ((int) (angles.x / 360) + 1);
   if (angles.x < -180)
      angles.x += 360 * ((int) (angles.x / 360) + 1);
   if (angles.y >= 180)
      angles.y -= 360 * ((int) (angles.y / 360) + 1);
   if (angles.y < -180)
      angles.y += 360 * ((int) (angles.y / 360) + 1);
   if (angles.z >= 180)
      angles.z -= 360 * ((int) (angles.z / 360) + 1);
   if (angles.z < -180)
      angles.z += 360 * ((int) (angles.z / 360) + 1);

   return (angles);
}


void UTIL_DrawBeam (Vector start, Vector end, int red, int green, int blue)
{
   if (FNullEnt (pListenserverEdict))
      return; // reliability check

   MESSAGE_BEGIN (MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, pListenserverEdict);
   WRITE_BYTE (TE_BEAMPOINTS);
   WRITE_COORD (start.x);
   WRITE_COORD (start.y);
   WRITE_COORD (start.z);
   WRITE_COORD (end.x);
   WRITE_COORD (end.y);
   WRITE_COORD (end.z);
   WRITE_SHORT (beam_texture);
   WRITE_BYTE (1); // framestart
   WRITE_BYTE (10); // framerate
   WRITE_BYTE (1); // life in 0.1's
   WRITE_BYTE (10); // width
   WRITE_BYTE (0); // noise
   WRITE_BYTE (red); // r, g, b
   WRITE_BYTE (green); // r, g, b
   WRITE_BYTE (blue); // r, g, b
   WRITE_BYTE (255); // brightness
   WRITE_BYTE (0); // speed
   MESSAGE_END ();

   return;
}
