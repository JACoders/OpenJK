typedef enum //# soundChannel_e
{
	CHAN_AUTO,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # Auto-picks an empty channel to play sound on
	CHAN_LOCAL,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # menu sounds, etc
	CHAN_WEAPON,//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" 
	CHAN_VOICE, //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Voice sounds cause mouth animation
	CHAN_VOICE_ATTEN, //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Causes mouth animation but still use normal sound falloff 
	CHAN_ITEM,  //## %s !!"W:\game\base\!!sound\*.wav;*.mp3"
	CHAN_BODY,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3"
	CHAN_AMBIENT,//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # added for ambient sounds
	CHAN_LOCAL_SOUND,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #chat messages, etc
	CHAN_ANNOUNCER,		//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #announcer voices, etc
	CHAN_LESS_ATTEN,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #attenuates similar to chan_voice, but uses empty channel auto-pick behaviour
	CHAN_MENU1,		//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #menu stuff, etc
	CHAN_VOICE_GLOBAL,  //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Causes mouth animation and is broadcast, like announcer
} soundChannel_t;

