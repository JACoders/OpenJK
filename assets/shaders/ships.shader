//Shader fix by TnG, adapted to Eternaljk by Art

models/map_objects/ships/cart
{
    {
        map $lightmap
        rgbGen identity
    }
    {
        map models/map_objects/ships/cart
        blendFunc GL_DST_COLOR GL_ZERO
        rgbGen identity
    }
}

models/map_objects/imp_mine/xwings
{
	q3map_nolightmap
    {
        map models/map_objects/imp_mine/xwings
        rgbGen vertex
    }
}

models/map_objects/imp_mine/xwglass_shd
{
	q3map_nolightmap
    {
        map models/map_objects/imp_mine/xwglass_shd
        rgbGen vertex
    }
}

models/map_objects/imp_mine/xwbody
{
	q3map_nolightmap
    {
        map models/map_objects/imp_mine/xwbody
        rgbGen vertex
    }
}
