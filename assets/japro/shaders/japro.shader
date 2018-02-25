bloodExplosion		// spurt of blood at point of impact
{
	cull disable
	nopicmip
	nomipmaps
	{
		animmap 5 gfx/japro/blood201.tga gfx/japro/blood202.tga gfx/japro/blood203.tga gfx/japro/blood204.tga gfx/japro/blood205.tga
		blendfunc blend
	}
}

bloodTrail
{
        
	nopicmip			// make sure a border remains
	entityMergable		// allow all the sprites to be merged together
	{
		//clampmap gfx/misc/blood.tga
                clampmap gfx/damage/blood_spurt.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}

bloodMark
{
	nopicmip			// make sure a border remains
	polygonOffset
	{
		clampmap gfx/damage/blood_stain.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identityLighting
		alphaGen vertex
	}
}

gfx/effects/sabers/RGBGlow
{	
	nopicmip
	cull	twosided
    {
        map gfx/effects/sabers/RGBGlow
        blendFunc GL_ONE GL_ONE
        glow
        
        rgbGen vertex
    }
}

gfx/effects/sabers/RGBCore
{
	nopicmip
	cull	twosided
    {
        map gfx/effects/sabers/RGBCore
        blendFunc GL_ONE GL_ONE
        
        rgbGen vertex
    }
}

gfx/2d/numbers/zero
{
	nopicmip
    {
       	map gfx/2d/numbers/zero
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/one
{
	nopicmip
    {
      	map gfx/2d/numbers/one
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/two
{
	cull twosided
	nopicmip
    {
        	map gfx/2d/numbers/two
       	 blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/three
{
	nopicmip
    {
        	map gfx/2d/numbers/three
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/four
{
	nopicmip
    {
       	 map gfx/2d/numbers/four
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/five
{
	nopicmip
    {
        	map gfx/2d/numbers/five
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/six
{
	nopicmip
    {
       	map gfx/2d/numbers/six
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/seven
{
	nopicmip
    {
        	map gfx/2d/numbers/seven
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/eight
{
	nopicmip
    {
        	map gfx/2d/numbers/eight
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/numbers/nine
{
	nopicmip
    {
        	map gfx/2d/numbers/nine
	blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/2d/minimap
{
	nopicmip
	notc
	q3map_nolightmap
    {
        map gfx/2d/minimap
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/effects/grapple_line
{
	cull	twosided
    {
        map gfx/effects/grapple_line
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/raceShader
{
    q3map_nolightmap
    {
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

