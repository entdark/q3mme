mme/fx/particle1
{
	{
		map data/particle1.tga
		blendFunc blend
		rgbgen vertex
		alphagen vertex
	}
}

mme/fx/particle2
{
	{
		map data/particle2.tga
		blendFunc blend
		rgbgen vertex
		alphagen vertex
	}
}

mme/fx/flare1
{
	{
		map data/flare1.tga
		blendFunc add
		rgbgen vertex
		alphagen vertex
	}
}


mme_additiveWhite
{
	{
		map *white
		blendfunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen vertex
	}
}

mme_df_unavailableItem
{
	{
		map textures/effects/quadmap2.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
	}
}


mme/menu/scrollFont
{	
	cull	none
	{
		clampmap menu/art/font1_prop.tga
		depthwrite
		alphafunc GT0
		blendfunc blend
		alphagen vertex
		rgbgen vertex
	}

}

mme/menu/scrollFontShadow
{
	cull	none
	{
		clampmap menu/art/font1_prop.tga
		depthwrite
		alphafunc GT0
		blendfunc gl_zero gl_src_color
		alphagen vertex
		rgbgen const ( 0.5 0.5 0.5 )
	}

}


mme/menu/boxCenterTop
{
	{
		map textures/base_floor/diamond2c.tga
		blendfunc GL_ONE GL_ZERO
		rgbGen identity
		alphaGen identity
	}
}

mme/menu/boxCenterSide
{
	{
		map textures/base_trim/dirty_pewter.tga
		blendfunc GL_ONE GL_ZERO
		rgbGen identity
		alphaGen identity
	}
}


mme/menu/star
{
	cull none
//        {
//		map textures/effects/tinfx.tga
//		blendFunc GL_ONE GL_ZERO
//                tcGen environment
//		rgbGen entity
//	}
        {
		map *white
		blendFunc blend
		rgbGen entity
		alphagen identity
	}
}

mme/menu/darken
{
	{
		map *white
		blendFunc GL_ZERO GL_SRC_ALPHA
		alphagen vertex
	}
}

mme/menu/background
{
	//Bit of an mme hack to disable all writing to depth buffer for an entire shader
	depthdisable
	{
		
		map *white
		blendfunc gl_one gl_zero
		rgbGen const ( 0.5 0.5 0.5 )
	}
	{
		map data/circlegradlightningblur3.tga
		blendfunc add
		rgbGen wave noise 0 1 0 4 
		tcMod rotate -11
		tcMod scale 0.5 0.5
		tcMod scroll 0.01 0.01
		tcMod scale 0.6 0.6
	}
	{
		map data/dense_blue_nebula.tga
		blendfunc filter
		tcMod rotate -10
		tcMod scroll 0.02 0.03
	}
}


mme/whitevertex
{
	{
		tcgen base
		map *white
		rgbgen exactVertex
	}
}

mme/whitediffuse
{
	{
		tcgen base
		map *white
		rgbgen lightingdiffuse
	}
}

mme/black
{
	{
		map *white
		rgbgen const ( 0 0 0 )
	}
}

mme/red
{
	cull	none
	{
		map *white
		rgbgen const ( 1 0 0 )
	}
}

mme/green
{
	{
		map *white
		rgbgen const ( 0 1 0 )
	}
}


mme/blue
{
	{
		map *white
		rgbgen const ( 0 0 1 )
	}
}


mme/pip
{
	{
		videomap data/pip.tga
		tcgen base
		tcmod rotate 10
//		tcMod stretch sin 1 0.2 0 0.3
	}
}


mme/gridline
{
	cull	none
	{
		map data/gridline.tga
		blendfunc add
		rgbgen vertex
		alphagen identity
	}
}
