from . import material

def build_blend_mode(
    cyc1: material.BlendModeCycle, 
    z_mode: str = None,
    z_write: bool = False,
    z_compare: bool = False,
    aa: bool = False,
    alpha_compare: str = None,
    coverage_dest: str = None,
    color_on_coverage: bool = False,
    x_coverage_alpha: bool = False,
    alpha_coverage: bool = False,
    force_blend: bool = False,
    image_read: bool = False
):
	return material.BlendMode(
		cyc1, 
		None, 
		z_mode=z_mode, 
		z_write=z_write, 
		z_compare=z_compare,
		aa=aa,
		alpha_compare=alpha_compare,
		coverage_dest=coverage_dest,
		color_on_coverage=color_on_coverage,
		x_coverage_alpha=x_coverage_alpha,
		alpha_coverage=alpha_coverage,
		force_blend=force_blend,
		image_read=image_read
	)

RM_AA_ZB_OPA_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, 
	z_compare = True, 
	z_write = True, 
	image_read = True, 
	coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', 
	alpha_coverage = True,
)
	

RM_RA_ZB_OPA_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, z_compare = True, z_write = True, coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', alpha_coverage = True
)

RM_AA_ZB_XLU_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, image_read = True, coverage_dest = 'WRAP',  color_on_coverage = True,
	force_blend = True, z_mode = 'TRANSPARENT',
)

RM_AA_ZB_OPA_DECAL = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, z_compare = True, image_read = True, coverage_dest = 'WRAP',  alpha_coverage = True,
	z_mode = 'DECAL',
)

RM_RA_ZB_OPA_DECAL = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, z_compare = True, coverage_dest = 'WRAP',  alpha_coverage = True,
	z_mode = 'DECAL',
)

RM_AA_ZB_XLU_DECAL = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, image_read = True, coverage_dest = 'WRAP',  color_on_coverage = True,
	force_blend = True, z_mode = 'DECAL'
)

RM_AA_ZB_OPA_INTER = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'CLAMP',
	alpha_coverage = True,	z_mode = 'INTER',
)

RM_RA_ZB_OPA_INTER = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, z_compare = True, z_write = True, coverage_dest = 'CLAMP',
	alpha_coverage = True,	z_mode = 'INTER',
)

RM_AA_ZB_XLU_INTER = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, image_read = True, coverage_dest = 'WRAP',  color_on_coverage = True,
	force_blend = True, z_mode = 'INTER',
)

RM_AA_ZB_XLU_LINE = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, image_read = True, coverage_dest = 'CLAMP', x_coverage_alpha = True,
	alpha_coverage = True, force_blend = True, z_mode = 'TRANSPARENT',
)

RM_AA_ZB_DEC_LINE = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, image_read = True, coverage_dest = 'SAVE', x_coverage_alpha = True,
	alpha_coverage = True, force_blend = True, z_mode = 'DECAL',
)

RM_AA_ZB_TEX_EDGE = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'MEM_A'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'CLAMP',
	x_coverage_alpha = True, alpha_coverage = True, z_mode = 'OPAQUE', 
)

RM_AA_ZB_TEX_INTER = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'MEM_A'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'CLAMP',
	x_coverage_alpha = True, alpha_coverage = True, z_mode = 'INTER', 
)

RM_AA_ZB_SUB_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'MEM_A'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'FULL',
	z_mode = 'OPAQUE', alpha_coverage = True,
)

RM_AA_ZB_PCL_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', alpha_compare = 'DITHER',
)

RM_AA_ZB_OPA_TERR = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', alpha_coverage = True,
)

RM_AA_ZB_TEX_TERR = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'CLAMP',
	x_coverage_alpha = True, alpha_coverage = True, z_mode = 'OPAQUE', 
)

RM_AA_ZB_SUB_TERR = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, z_compare = True, z_write = True, image_read = True, coverage_dest = 'FULL',
	z_mode = 'OPAQUE', alpha_coverage = True,
)


RM_AA_OPA_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, image_read = True, coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', alpha_coverage = True,
)

RM_RA_OPA_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	aa = True, coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', alpha_coverage = True,
)

RM_AA_XLU_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, image_read = True, coverage_dest = 'WRAP',  color_on_coverage = True, force_blend = True,
	z_mode = 'OPAQUE',
)

RM_AA_XLU_LINE = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, image_read = True, coverage_dest = 'CLAMP', x_coverage_alpha = True,
	alpha_coverage = True, force_blend = True, z_mode = 'OPAQUE',
)

RM_AA_DEC_LINE = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, image_read = True, coverage_dest = 'FULL', x_coverage_alpha = True,
	alpha_coverage = True, force_blend = True, z_mode = 'OPAQUE',
)

RM_AA_TEX_EDGE = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'MEM_A'),
	aa = True, image_read = True, coverage_dest = 'CLAMP',
	x_coverage_alpha = True, alpha_coverage = True, z_mode = 'OPAQUE', 
)

RM_AA_SUB_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'MEM_A'),
	aa = True, image_read = True, coverage_dest = 'FULL',
	z_mode = 'OPAQUE', alpha_coverage = True,
)

RM_AA_PCL_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, image_read = True, coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', alpha_compare = 'DITHER',
)

RM_AA_OPA_TERR = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, image_read = True, coverage_dest = 'CLAMP',
	z_mode = 'OPAQUE', alpha_coverage = True,
)

RM_AA_TEX_TERR = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, image_read = True, coverage_dest = 'CLAMP',
	x_coverage_alpha = True, alpha_coverage = True, z_mode = 'OPAQUE',
) 

RM_AA_SUB_TERR = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	aa = True, image_read = True, coverage_dest = 'FULL',
	z_mode = 'OPAQUE', alpha_coverage = True,
)


RM_ZB_OPA_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	z_compare = True, z_write = True, coverage_dest = 'FULL', alpha_coverage = True,
	z_mode = 'OPAQUE',
)
	
RM_ZB_XLU_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	z_compare = True, image_read = True, coverage_dest = 'FULL', force_blend = True, z_mode = 'TRANSPARENT',
)
	
RM_ZB_OPA_DECAL = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	z_compare = True, coverage_dest = 'FULL', alpha_coverage = True, z_mode = 'DECAL',
)
	
RM_ZB_XLU_DECAL = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	z_compare = True, image_read = True, coverage_dest = 'FULL', force_blend = True, z_mode = 'DECAL',
)
	
RM_ZB_CLD_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	z_compare = True, image_read = True, coverage_dest = 'SAVE', force_blend = True, z_mode = 'TRANSPARENT',
)
	
RM_ZB_OVL_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	z_compare = True, image_read = True, coverage_dest = 'SAVE', force_blend = True, z_mode = 'DECAL',
)
	
RM_ZB_PCL_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	z_compare = True, z_write = True, coverage_dest = 'FULL', z_mode = 'OPAQUE',
	alpha_compare = 'DITHER',
)


RM_OPA_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	coverage_dest = 'CLAMP', force_blend = True, z_mode = 'OPAQUE',
)

RM_XLU_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	image_read = True, coverage_dest = 'FULL', force_blend = True, z_mode = 'OPAQUE',
)

RM_TEX_EDGE = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	coverage_dest = 'CLAMP', x_coverage_alpha = True, alpha_coverage = True, force_blend = True,
	z_mode = 'OPAQUE',  aa = True,
)

RM_CLD_SURF = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'),
	image_read = True, coverage_dest = 'SAVE', force_blend = True, z_mode = 'OPAQUE',
)

RM_PCL_SURF = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	coverage_dest = 'FULL', force_blend = True, z_mode = 'OPAQUE',
	alpha_compare = 'DITHER',
)

RM_ADD = build_blend_mode(
	material.BlendModeCycle('IN', 'FOG_A', 'MEMORY', '1'),
	image_read = True, coverage_dest = 'SAVE', force_blend = True, z_mode = 'OPAQUE',
)

RM_NOOP = build_blend_mode(
	material.BlendModeCycle('IN', 'IN_A', 'IN', 'INV_MUX_A')
)

RM_VISCVG = build_blend_mode(
	material.BlendModeCycle('IN', '0', '1', 'MEM_A'),
	image_read = True, force_blend = True,
)

RM_OPA_CI = build_blend_mode(
	material.BlendModeCycle('IN', '0', 'IN', '1'),
	coverage_dest = 'CLAMP', z_mode = 'OPAQUE',
)

# there is a difference bewteen what libultra defines and what seems to work
# RM_FOG_SHADE_A = build_blend_mode(material.BlendModeCycle('FOG', 'SHADE_A', 'IN', 'INV_MUX_A'))
RM_FOG_SHADE_A = build_blend_mode(material.BlendModeCycle('IN', 'SHADE_A', 'FOG', 'INV_MUX_A'))
RM_FOG_PRIM_A	= build_blend_mode(material.BlendModeCycle('FOG', 'FOG_A', 'IN', 'INV_MUX_A'))
RM_PASS = build_blend_mode(material.BlendModeCycle('IN', '0', 'IN', '1'))

def combine_blend_mode(a: material.BlendMode, b: material.BlendMode | None):
	if not b:
		return material.BlendMode(
			a.cyc1,
			None,
			z_mode = a.z_mode or 'OPAQUE',
			z_write = a.z_write,
			z_compare = a.z_compare,
			aa = a.aa,
			alpha_compare = a.alpha_compare or 'NONE',
			coverage_dest = a.coverage_dest or 'CLAMP',
			color_on_coverage = a.color_on_coverage,
			x_coverage_alpha = a.x_coverage_alpha,
			alpha_coverage = a.alpha_coverage,
			force_blend = a.force_blend,
			image_read = a.image_read
		)


	return material.BlendMode(
		a.cyc1,
		b.cyc1,
        z_mode = a.z_mode or b.z_mode or 'OPAQUE',
        z_write = a.z_write or b.z_write,
        z_compare = a.z_compare or b.z_compare,
        aa = a.aa or b.aa,
        alpha_compare = a.alpha_compare or b.alpha_compare or 'NONE',
        coverage_dest = a.coverage_dest or b.coverage_dest or 'CLAMP',
        color_on_coverage = a.color_on_coverage or b.color_on_coverage,
        x_coverage_alpha = a.x_coverage_alpha or b.x_coverage_alpha,
        alpha_coverage = a.alpha_coverage or b.alpha_coverage,
        force_blend = a.force_blend or b.force_blend,
        image_read = a.image_read or b.image_read
    )