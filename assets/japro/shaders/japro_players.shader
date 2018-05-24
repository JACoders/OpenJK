//Artemis: following were added to fix model parts that were left out of base players shader
models/players/human_merc/human_merc_torso_lower
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/human_merc/human_merc_torso_lower
		rgbGen lightingDiffuse
	}
}

models/players/human_merc/human_merc_torso_lower_blue
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/human_merc/human_merc_torso_lower_blue
		rgbGen lightingDiffuse
	}
}

models/players/human_merc/human_merc_torso_lower_red
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/human_merc/human_merc_torso_lower_red
		rgbGen lightingDiffuse
	}
}

models/players/human_merc/racto_torso_lower
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/human_merc/racto_torso_lower
		rgbGen lightingDiffuse
	}
}

models/players/jedi_tf/torso_03_lower
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/jedi_tf/torso_03_lower
		rgbGen lightingDiffuse
	}
}

models/players/jedi_tf/torso_04_lower
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/jedi_tf/torso_04_lower
		rgbGen lightingDiffuse
	}
}

models/players/noghri/armor
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/noghri/armor
		rgbGen lightingDiffuse
	}
}

models/players/noghri/torso_lower
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/noghri/torso_lower
		rgbGen lightingDiffuse
	}
}

models/players/noghri/torso_lower_blue
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/noghri/torso_lower_blue
		rgbGen lightingDiffuse
	}
}

models/players/noghri/torso_lower_red
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/noghri/torso_lower_red
		rgbGen lightingDiffuse
	}
}

models/players/rax_joris/robe
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/rax_joris/robe
		rgbGen lightingDiffuse
	}
}

models/players/rax_joris/robe_blue
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/rax_joris/robe_blue
		rgbGen lightingDiffuse
	}
}

models/players/rax_joris/robe_red
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/rax_joris/robe_red
		rgbGen lightingDiffuse
	}
}

models/players/reborn/boss_flap
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/reborn/boss_flap
		rgbGen lightingDiffuse
	}
}

models/players/reborn/flap
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/reborn/flap
		rgbGen lightingDiffuse
	}
}

models/players/reborn/flap_blue
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/reborn/flap_blue
		rgbGen lightingDiffuse
	}
}

models/players/reborn/forc_flap
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/reborn/forc_flap
		rgbGen lightingDiffuse
	}
}

models/players/rosh_penin/flaps
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/rosh_penin/flaps
		rgbGen lightingDiffuse
	}
}

models/players/rosh_penin/flaps_blue
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/rosh_penin/flaps_blue
		rgbGen lightingDiffuse
	}
}

models/players/rosh_penin/flaps_red
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/rosh_penin/flaps_red
		rgbGen lightingDiffuse
	}
}

models/players/saboteur/saboteur_torso_lower
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/saboteur/saboteur_torso_lower
		rgbGen lightingDiffuse
	}
}

models/players/saboteur/saboteur_torso_lower_blue
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/saboteur/saboteur_torso_lower_blue
		rgbGen lightingDiffuse
	}
}

models/players/saboteur/saboteur_torso_lower_red
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/saboteur/saboteur_torso_lower_red
		rgbGen lightingDiffuse
	}
}

models/players/tavion/feathers
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/tavion/feathers
		rgbGen lightingDiffuse
	}
}

models/players/tavion_new/feathers
{
	q3map_nolightmap
	cull	twosided
	{
		map models/players/tavion_new/feathers
		rgbGen lightingDiffuse
	}
}

//VFX Duelers/Racers
gfx/effects/raceShader
{
	{
	map gfx/mp/forceshell
	blendFunc GL_ONE GL_ONE
    rgbGen const ( 0.1 0.1 1 )
	//rgb num divided by 255
	tcGen environment
	tcMod rotate 25
	tcMod turb 0.4 0.2 0 0.4
	tcMod scale 0.2 0.2
	}
/*	{ 
		map textures/colors/red 
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA 
		depthWrite 
		rgbGen lightingDiffuse 
		alphaGen wave sin 0.1 0.1 0.1 0.1 
	}
	{ 
		map textures/colors/blue 
		blendFunc GL_ONE GL_ONE 
		rgbGen wave sin 0.9 0.1 0.1 0.1 
	}
*/
}

gfx/effects/duelShader
{
	{
		map gfx/effects/plasma
		blendFunc GL_DST_COLOR GL_ONE
		rgbGen entity
		tcGen environment
		tcMod scroll 0.3 0.2
		tcMod turb 0.6 0.3 0 0.2
	}
}

//RGB Players
models/players/jedi/tint_torso
{
	{
		map models/players/jedi/tint_torso
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi/tint_torso
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/jedi/tint_legs
{
	{
		map models/players/jedi/tint_legs
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi/tint_legs
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/rebel_pilot/tint_rebel_pilot_legs
{
	{
		map models/players/rebel_pilot/tint_rebel_pilot_legs
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/rebel_pilot/tint_rebel_pilot_legs
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}


models/players/rebel_pilot/tint_rebel_pilot_torso
{
	{
		map models/players/rebel_pilot/tint_rebel_pilot_torso
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/rebel_pilot/tint_rebel_pilot_torso
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/jan/tint_torso
{
	{
		map models/players/jan/tint_torso
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jan/tint_torso
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/jan/tint_legs
{
	{
		map models/players/jan/tint_legs
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jan/tint_legs
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/jan/tint_accesories
{
	{
		map models/players/jan/tint_accesories
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jan/tint_accesories
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/jan/tint_vest
{
	{
		map models/players/jan/tint_vest
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jan/tint_vest
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}


models/players/imperial_worker/tint_torso
{
	{
		map models/players/imperial_worker/tint_torso
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/imperial_worker/tint_torso
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/imperial_worker/tint_leg
{
	{
		map models/players/imperial_worker/tint_leg
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/imperial_worker/tint_leg
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/imperial_worker/tint_helmet_beltpac
{
	{
		map models/players/imperial_worker/tint_helmet_beltpac
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/imperial_worker/tint_helmet_beltpac
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		detail
		rgbGen lightingDiffuse
	}
}

models/players/jedi_hoth/male_arms_rgb
{
	cull	twosided
	{
		map models/players/jedi_hoth/male_arms_rgb
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi_hoth/male_arms_rgb
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
}

models/players/jedi_hoth/male_legs_rgb
{
	cull	twosided
	{
		map models/players/jedi_hoth/male_legs_rgb
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi_hoth/male_legs_rgb
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
}

models/players/jedi_hoth/male_torso_rgb
{
	{
		map models/players/jedi_hoth/male_torso_rgb
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi_hoth/male_torso_rgb
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
}

models/players/jedi_hoth/female_arms_rgb
{
	cull	twosided
	{
		map models/players/jedi_hoth/female_arms_rgb
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi_hoth/female_arms_rgb
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
}

models/players/jedi_hoth/female_legs_rgb
{
	cull	twosided
	{
		map models/players/jedi_hoth/female_legs_rgb
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi_hoth/female_legs_rgb
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
}

models/players/jedi_hoth/female_torso_rgb
{
	cull	twosided
	{
		map models/players/jedi_hoth/female_torso_rgb
		rgbGen lightingDiffuseEntity
	}
	{
		map models/players/jedi_hoth/female_torso_rgb
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
}