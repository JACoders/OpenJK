gfx/effects/sabers/RGBglow1 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBglow1
		blendFunc GL_ONE GL_ONE
		glow
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBcore1 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBcore1
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBglow2 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBglow2
		blendFunc GL_ONE GL_ONE
		glow
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBcore2 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBcore2
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBglow3 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBglow3
		blendFunc GL_ONE GL_ONE
		glow
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBcore3 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBcore3
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBglow4 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBglow4
		blendFunc GL_ONE GL_ONE
		glow
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBcore4 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBcore4
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBglow5 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBglow5
		blendFunc GL_ONE GL_ONE
		glow
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBcore5 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBcore5
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/blackglow {
	nopicmip
	cull twosided
	{
		clampmap gfx/effects/sabers/blackglow
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
		alphaGen vertex
	}
}

gfx/effects/sabers/blackcore {
	nopicmip
	cull twosided
	{
		clampmap gfx/effects/sabers/blackcore
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
		alphaGen vertex
	}
}

gfx/effects/sabers/blacktrail {
	nopicmip
	cull twosided
	{
		clampmap gfx/effects/sabers/blacktrail
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
		alphaGen vertex
	}
	{
		clampmap gfx/effects/sabers/blacktrail
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
		alphaGen vertex
	}
}

gfx/effects/sabers/RGBtrail2 {
	nopicmip
	cull disable
	{
		clampmap gfx/effects/sabers/RGBtrail2
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
	{
		clampmap gfx/effects/sabers/RGBtrail2
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

gfx/effects/sabers/RGBtrail3 {
	nopicmip
	cull disable
	{
		clampmap gfx/effects/sabers/RGBtrail3
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
	{
		clampmap gfx/effects/sabers/RGBtrail3
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

gfx/effects/sabers/RGBglow4 {
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/RGBglow4
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/RGBtrail4 {
	nopicmip
	cull disable
	{
		clampmap gfx/effects/sabers/RGBtrail4
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
	{
		clampmap gfx/effects/sabers/RGBtrail4
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

gfx/effects/sabers/saber_trail {
	nopicmip
	cull twosided
	{
		map $whiteimage
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

gfx/effects/sabers/saber_blade {
	notc
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/saber_blade
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/saber_blade_rgb {
	notc
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/saber_blade
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

gfx/effects/sabers/saber_end {
	notc
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/saber_end
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sabers/saber_end_rgb {
	notc
	nopicmip
	cull twosided
	{
		map gfx/effects/sabers/saber_end
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}

