/******************************************************************************
 * Copyright 1986-2008 by mental images GmbH, Fasanenstr. 81, D-10623 Berlin,
 * Germany. All rights reserved.
 ******************************************************************************
 * Created:	20.10.97
 * Module:	baseshader
 * Purpose:	base shaders for Phenomenon writers
 *
 * Exports:
 *
 *      mib_illum_phong()
 *
 * History:
 *      20.10.97: initial version
 *	17.11.97: added ambience parameter
 *	27.01.98: added global illumination capabilities
 *
 * Description:
 *      Perform illumination with the following reflection model:
 *      - Phong specular (cosine power) plus
 *      - Lambert diffuse (cosine) plus
 *      - ambient (constant)
 *****************************************************************************/

#ifdef HPUX
#pragma OPT_LEVEL 1	/* workaround for HP/UX optimizer bug, +O2 and +O3 */
#endif

#include <stdio.h>
#include <stdlib.h>		/* for abs */
#include <float.h>		/* for FLT_MAX */
#include <math.h>
#include <string.h>
#include <assert.h>
#include "shader.h"
#include "mi_shader_if.h"

struct mib_illum_phong {
	miColor		ambience;	/* ambient color multiplier */
	miColor		ambient;	/* ambient color */
	miColor		diffuse;	/* diffuse color */
	miColor		specular;	/* specular color */
	miScalar	exponent;	/* shinyness */
	int		mode;           /* light mode: 0..2 */
	int		i_light;	/* index of first light */
	int		n_light;	/* number of lights */
	miTag		light[1];	/* list of lights */
};


extern "C" DLLEXPORT int mib_illum_phong_version(void) {return(2);}

extern "C" DLLEXPORT miBoolean mib_illum_phong(
	miColor		*result,
	miState		*state,
	struct mib_illum_phong *paras)
{
	miColor		*ambi, *diff, *spec;
	miTag		*light;		/* tag of light instance */
	int		n_l;		/* number of light sources */
	int		i_l;		/* offset of light sources */
	int		m;		/* light mode: 0=all, 1=incl, 2=excl */
	int		samples;	/* # of samples taken */
	miColor		color;		/* color from light source */
	miColor		sum;		/* summed sample colors */
	miVector	dir;		/* direction towards light */
	miScalar	dot_nl;		/* dot prod of normal and dir */
	miScalar	expo;		/* Phong exponent (cosine power) */
	miScalar	s;		/* amount of specular reflection */
       
        /* check for illegal calls */
        if (state->type == miRAY_SHADOW || state->type == miRAY_DISPLACE ) {
		return(miFALSE);
	}
 
	ambi =  mi_eval_color(&paras->ambient);
	diff =  mi_eval_color(&paras->diffuse);
	spec =  mi_eval_color(&paras->specular);
	expo = *mi_eval_scalar(&paras->exponent);
	m    = *mi_eval_integer(&paras->mode);

	*result    = *mi_eval_color(&paras->ambience);	/* ambient term */
	result->r *= ambi->r;
	result->g *= ambi->g;
	result->b *= ambi->b;

	n_l   = *mi_eval_integer(&paras->n_light);
	i_l   = *mi_eval_integer(&paras->i_light);
	light =  mi_eval_tag(paras->light) + i_l;

	if (m == 1)		/* modify light list (inclusive mode) */
		mi_inclusive_lightlist(&n_l, &light, state);
	else if (m == 2)	/* modify light list (exclusive mode) */
		mi_exclusive_lightlist(&n_l, &light, state);
	else if (m == 4) {
		n_l = 0;
		light = 0;
	}

	/* Loop over all light sources */
	if (m == 4 || n_l) {
		for (mi::shader::LightIterator iter(state, light, n_l);
						!iter.at_end(); ++iter) {
			sum.r = sum.g = sum.b = 0;
			while (iter->sample()) {
				/* Lambert's cosine law */
				dot_nl = iter->get_dot_nl();
				dir = iter->get_direction();
				iter->get_contribution(&color);
				sum.r += dot_nl * diff->r * color.r;
				sum.g += dot_nl * diff->g * color.g;
				sum.b += dot_nl * diff->b * color.b;

				/* Phong's cosine power */
				s = mi_phong_specular(expo, state, &dir);
				if (s > 0.0) {
					sum.r += s * spec->r * color.r;
					sum.g += s * spec->g * color.g;
					sum.b += s * spec->b * color.b;
				}
			}
			samples = iter->get_number_of_samples();
			if (samples) {
				result->r += sum.r / samples;
				result->g += sum.g / samples;
				result->b += sum.b / samples;
			}
		}
	}
	

	/* add contribution from indirect illumination (caustics) */
	mi_compute_irradiance(&color, state);
	result->r += color.r * diff->r;
	result->g += color.g * diff->g;
	result->b += color.b * diff->b;
	result->a  = 1;
	return(miTRUE);
}
