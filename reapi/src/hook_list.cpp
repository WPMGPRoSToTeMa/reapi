#include "precompiled.h"

inline size_t getFwdParamType(void(*)(int))				{ return FP_CELL; }
inline size_t getFwdParamType(void(*)(bool))			{ return FP_CELL; }
inline size_t getFwdParamType(void(*)(Vector&))			{ return FP_CELL; }
inline size_t getFwdParamType(void(*)(PLAYER_ANIM))		{ return FP_CELL; }
inline size_t getFwdParamType(void(*)(float))			{ return FP_FLOAT; }
inline size_t getFwdParamType(void(*)(const char *))	{ return FP_STRING; }

template<typename T>
inline size_t getFwdParamType(void(*)(T *))				{ return FP_CELL; }

template<size_t current = 0>
static void setupParamTypes(size_t param_types[], void (*)())
{
	param_types[current] = FP_DONE;
}

template<size_t current = 0, typename T, typename ...t_args>
static void setupParamTypes(size_t param_types[], void(*)(T, t_args...))
{
	void (*func)(T) = nullptr;
	param_types[current] = getFwdParamType(func);
	void (*next)(t_args...) = nullptr;
	setupParamTypes<current + 1>(param_types, next);
}

template<typename... f_args>
struct regargs
{
	regargs(void(*)(f_args...))
	{
		void (*func)(f_args...) = nullptr;
		setupParamTypes(types, func);
	}

	size_t types[sizeof...(f_args)+1]; // + FP_DONE
};

struct regfunc
{
	template<typename R, typename T, typename... f_args>
	regfunc(R (*)(T, f_args...))
	{
		func = [](AMX *amx, const char *name) {
			void(*func)(f_args...) = nullptr;
			regargs<f_args...> args(func);
			return g_amxxapi.RegisterSPForwardByName(amx, name, args);
		};
	}

	regfunc(const char *error) { UTIL_SysError(error); }	// to cause a amxx module failure.
	operator regfunc_t() const { return func; }
	regfunc_t func;

	static int current_cell;				// the counter of cells
};

int regfunc::current_cell = 1;

#define ENG(h) { {}, {}, #h, "ReHLDS", [](){ return api_cfg.hasReHLDS(); }, ((!(RH_##h & (MAX_REGION_RANGE - 1)) ? regfunc::current_cell = 1, true : false) || (RH_##h & (MAX_REGION_RANGE - 1)) == regfunc::current_cell++) ? regfunc(h) : regfunc(#h " doesn't match hook definition"), [](){ g_RehldsHookchains->##h##()->registerHook(&##h); }, [](){ g_RehldsHookchains->##h##()->unregisterHook(&##h); }}
hook_t hooklist_engine[] = {
	ENG(SV_StartSound),
	ENG(SV_DropClient),
	ENG(SV_ActivateServer),
	ENG(Cvar_DirectSet)
};

#define DLL(h) { {}, {}, #h, "ReGameDLL", [](){ return api_cfg.hasReGameDLL(); }, ((!(RH_##h & (MAX_REGION_RANGE - 1)) ? regfunc::current_cell = 1, true : false) || (RH_##h & (MAX_REGION_RANGE - 1)) == regfunc::current_cell++) ? regfunc(h) : regfunc(#h " doesn't match hook definition"), [](){ g_ReGameHookchains->##h##()->registerHook(&##h); }, [](){ g_ReGameHookchains->##h##()->unregisterHook(&##h); }}
hook_t hooklist_gamedll[] = {
	DLL(GetForceCamera),
	DLL(PlayerBlind),
	DLL(RadiusFlash_TraceLine)
};

hook_t hooklist_animating[] = {
	DLL(CBaseAnimating_ResetSequenceInfo)
};

hook_t hooklist_player[] = {
	DLL(CBasePlayer_Spawn),
	DLL(CBasePlayer_Precache),
	DLL(CBasePlayer_ObjectCaps),
	DLL(CBasePlayer_Classify),
	DLL(CBasePlayer_TraceAttack),
	DLL(CBasePlayer_TakeDamage),
	DLL(CBasePlayer_TakeHealth),
	DLL(CBasePlayer_Killed),
	DLL(CBasePlayer_AddPoints),
	DLL(CBasePlayer_AddPointsToTeam),
	DLL(CBasePlayer_AddPlayerItem),
	DLL(CBasePlayer_RemovePlayerItem),
	DLL(CBasePlayer_GiveAmmo),
	DLL(CBasePlayer_ResetMaxSpeed),
	DLL(CBasePlayer_Jump),
	DLL(CBasePlayer_Duck),
	DLL(CBasePlayer_PreThink),
	DLL(CBasePlayer_PostThink),
	DLL(CBasePlayer_UpdateClientData),
	DLL(CBasePlayer_ImpulseCommands),
	DLL(CBasePlayer_RoundRespawn),
	DLL(CBasePlayer_Blind),

	DLL(CBasePlayer_Observer_IsValidTarget),
	DLL(CBasePlayer_SetAnimation),
	DLL(CBasePlayer_GiveDefaultItems),
	DLL(CBasePlayer_GiveNamedItem),
	DLL(CBasePlayer_AddAccount),
	DLL(CBasePlayer_GiveShield),
};

hook_t* hooklist_t::getHookSafe(size_t hook)
{
	#define CASE(h)	case ht_##h: if (index < arraysize(hooklist_##h)) return &hooklist_##h[index]; else break;

	const auto table = hooks_tables_e(hook / MAX_REGION_RANGE);
	const auto index = hook & (MAX_REGION_RANGE - 1);

	switch (table) {
		CASE(engine)
		CASE(gamedll)
		CASE(animating)
		CASE(player)
	}

	return nullptr;
}

void hook_t::clear()
{
	for (auto h : pre)
		delete h;
	pre.clear();

	for (auto h : post)
		delete h;
	post.clear();

	unregisterHookchain();
}
