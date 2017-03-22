/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* 'knob' gui object by Frank Barknecht, 'externalised' by Olaf Matthes */
/* changed for devel_0.37 by Christoph Kummerer - hardly tested         */

/* I had to out-comment the loadbang stuff because I couldn't find the code
   in Pd where it is declared */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"

#ifndef PD_MAJOR_VERSION
#include "s_stuff.h"
#else 
#include "m_imp.h"
#endif

#include "g_canvas.h"

#include "../../old_g_all_guis.inc"
#include <math.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.141592654
#endif

#define IEM_KNOB_DEFAULTSIZE 32

/* ------------ knob gui-vertical  slider ----------------------- */

t_widgetbehavior knob_widgetbehavior;
static t_class *knob_class;
t_symbol *iemgui_key_sym=0;		/* taken from g_all_guis.c */

typedef struct _knob			/* taken from Frank's modyfied g_all_guis.h */
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_lin0_log1;
    int      x_steady;
    double   x_min;
    double   x_max;
    double   x_k;
} t_knob;

/* widget helper functions */

static void knob_draw_update(t_knob *x, t_glist *glist)
{
    if (glist_isvisible(glist))
    {
	/* compute dial:*/ 
	float radius = 0.5*(float)x->x_gui.x_h;
	double angle = 7.0/36.0 + 34.0/36.0*2.0*M_PI*( (double)x->x_val*0.01/(double)x->x_gui.x_h );
	int start = -80;
	int extent = 350 - (int)(360.0*angle/(2.0*M_PI));
	/* center point: */
	int x1 = text_xpix(&x->x_gui.x_obj, glist) + radius;
	int y1 = text_ypix(&x->x_gui.x_obj, glist) + radius;
	int x2 = text_xpix(&x->x_gui.x_obj, glist) + radius + radius * sin( -angle); 
        int y2 = text_ypix(&x->x_gui.x_obj, glist) + radius + radius * cos(  angle);
	
	sys_vgui(".x%lx.c coords %xKNOB %d %d %d %d\n",
		glist_getcanvas(glist), x,
		x1,  /* x1 */     
		y1,  /* y1 */  
		x2,  /* x2 */
		y2   /* y2 */
		);
        
    	/* post("knob: (%d, %d) (%d, %d)", x1,y1,x2,y2); */
	
	sys_vgui(".x%lx.c itemconfigure %xBASE -start %d -extent %d \n", glist_getcanvas(glist), x,
	     start, extent);
    }
}

static void knob_draw_new(t_knob *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    /* int r = ypos + x->x_gui.x_h - (x->x_val + 50)/100; */
    t_canvas *canvas=glist_getcanvas(glist);
    
    /* compute dial:*/ 
    float radius = 0.5*(float)x->x_gui.x_h;
    /* center point: */
    int x1 = xpos + radius;
    int y1 = ypos + radius;
    /* dial */  
    double angle = 7.0/36.0 + 34.0/36.0*2.0*M_PI*((float)x->x_val*0.01/(float)(x->x_gui.x_h));
    int x2 = x1 + radius * sin(-angle);
    int y2 = y1 + radius * cos(angle);
    

    /* BASE2 */
    sys_vgui(".x%lx.c create arc %d %d %d %d -outline #%6.6x -style arc -width 3 -start -80 -extent 340 -tags %xBASE2\n",
	     canvas, 
	     xpos, ypos, 				/*  upper left */
	     xpos + x->x_gui.x_h, ypos + x->x_gui.x_h,	/* lower right */
	     x->x_gui.x_fcol, x);
    
    /* BASE */
    sys_vgui(".x%lx.c create arc %d %d %d %d -fill #%6.6x -style arc -width 3 -start -80 -extent 340 -tags %xBASE\n",
	     canvas, 
	     xpos, ypos, 				/*  upper left */
	     xpos + x->x_gui.x_h, ypos + x->x_gui.x_h,	/* lower right */
	     x->x_gui.x_bcol, x);
    /* LINE */
    sys_vgui(".x%lx.c create line %d %d %d %d -width 3 -fill #%6.6x -capstyle round -tags %xKNOB\n",
	     canvas, 
	     x1,  	/* x1 */     
	     y1,  	/* y1 */  
	     x2,  			/* x2 */
	     y2,                        /* y2 */
	     x->x_gui.x_fcol,          	/* color */
	     x);   
    
    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
	     -font {%s %d bold} -fill #%6.6x -tags %xLABEL\n",
	     canvas, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy,
	     strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
	     x->x_gui.x_font, x->x_gui.x_fontsize, x->x_gui.x_lcol, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %xOUT%d\n",
	     canvas,
	     xpos, ypos + x->x_gui.x_h+2,
	     xpos+7, ypos + x->x_gui.x_h+3,
	     x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
	sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %xIN%d\n",
	     canvas,
	     xpos, ypos-2,
	     xpos+7, ypos-1,
	     x, 0);
}

static void knob_draw_move(t_knob *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    /* int r = ypos + x->x_gui.x_h - (x->x_val + 50)/100; */
    t_canvas *canvas=glist_getcanvas(glist);
    /* compute dial:*/ 
    float radius = 0.5*(float)x->x_gui.x_h;
    /* float angle = 7.0/36.0 + 34.0/36.0*2.0*M_PI*((float)x->x_val*0.01/(float)(x->x_gui.x_h)); */
    double angle = 7.0/36.0 + 34.0/36.0*2.0*M_PI*( (double)x->x_val*0.01/(double)x->x_gui.x_h );
    /* center point: */
    int x1 = text_xpix(&x->x_gui.x_obj, glist) + radius;
    int y1 = text_ypix(&x->x_gui.x_obj, glist) + radius;
    int x2 = text_xpix(&x->x_gui.x_obj, glist) + radius + radius * sin( -angle); 
    int y2 = text_ypix(&x->x_gui.x_obj, glist) + radius + radius * cos(  angle);
	
   
    
    sys_vgui(".x%lx.c coords %xKNOB %d %d %d %d\n",
	     canvas, x, 
	     x1,  /* x1 */     
	     y1,  /* y1 */  
	     x2,  /* x2 */
	     y2  /* y2 */
	     );
    /* post("knob: (%d, %d) (%d, %d)", x1,y1,x2,y2); */
    
    sys_vgui(".x%lx.c coords %xBASE %d %d %d %d\n",
	     canvas, x,
	     xpos, ypos,
	     xpos + x->x_gui.x_h, ypos + x->x_gui.x_h);
    
    sys_vgui(".x%lx.c coords %xBASE2 %d %d %d %d\n",
	     canvas, x,
	     xpos, ypos,
	     xpos + x->x_gui.x_h, ypos + x->x_gui.x_h);
    
    sys_vgui(".x%lx.c coords %xLABEL %d %d\n",
	     canvas, x, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c coords %xOUT%d %d %d %d %d\n",
	     canvas, x, 0,
	     xpos, ypos + x->x_gui.x_h+2,
	     xpos+7, ypos + x->x_gui.x_h+3);
    if(!x->x_gui.x_fsf.x_rcv_able)
	sys_vgui(".x%lx.c coords %xIN%d %d %d %d %d\n",
	     canvas, x, 0,
	     xpos, ypos-2,
	     xpos+7, ypos-1);
}

static void knob_draw_erase(t_knob* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c delete %xBASE\n", canvas, x);
    sys_vgui(".x%lx.c delete %xBASE2\n", canvas, x);
    sys_vgui(".x%lx.c delete %xKNOB\n", canvas, x);
    sys_vgui(".x%lx.c delete %xLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %xOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
	sys_vgui(".x%lx.c delete %xIN%d\n", canvas, x, 0);
}

static void knob_draw_config(t_knob* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c itemconfigure %xLABEL -font {%s %d bold} -fill #%6.6x -text {%s} \n",
	     canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize,
	     x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
	     strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%lx.c itemconfigure %xKNOB -fill #%6.6x\n", canvas,
	     x, x->x_gui.x_fcol);
    sys_vgui(".x%lx.c itemconfigure %xBASE2 -outline #%6.6x\n", canvas,
	     x, x->x_gui.x_fcol);
    sys_vgui(".x%lx.c itemconfigure %xBASE -fill #%6.6x\n", canvas,
	     x, x->x_gui.x_bcol);
}

static void knob_draw_io(t_knob* x,t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %xOUT%d\n",
	     canvas,
	     xpos, ypos + x->x_gui.x_h+2,
	     xpos+7, ypos + x->x_gui.x_h+3,
	     x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %xOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %xIN%d\n",
	     canvas,
	     xpos, ypos-2,
	     xpos+7, ypos-1,
	     x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %xIN%d\n", canvas, x, 0);
}

static void knob_draw_select(t_knob *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(x->x_gui.x_fsf.x_selected)
    {
	pd_bind(&x->x_gui.x_obj.ob_pd, iemgui_key_sym);
	sys_vgui(".x%lx.c itemconfigure %xBASE2 -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
	sys_vgui(".x%lx.c itemconfigure %xBASE -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
	sys_vgui(".x%lx.c itemconfigure %xLABEL -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
    }
    else
    {
	pd_unbind(&x->x_gui.x_obj.ob_pd, iemgui_key_sym);
	sys_vgui(".x%lx.c itemconfigure %xBASE -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
	sys_vgui(".x%lx.c itemconfigure %xBASE2 -outline #%6.6x\n", canvas, x, x->x_gui.x_fcol);
	sys_vgui(".x%lx.c itemconfigure %xLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
    }
}

void knob_draw(t_knob *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
	knob_draw_update(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
	knob_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
	knob_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
	knob_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
	knob_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
	knob_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
	knob_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ knob widgetbehaviour----------------------------- */


static void knob_getrect(t_gobj *z, t_glist *glist,
			    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_knob* x = (t_knob*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist) - 2;
    *xp2 = *xp1 + x->x_gui.x_h;
    *yp2 = *yp1 + x->x_gui.x_h + 5;
}

static void knob_save(t_gobj *z, t_binbuf *b)
{
    t_knob *x = (t_knob *)z;
    int bflcol[3], *ip1, *ip2;
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    ip1 = (int *)(&x->x_gui.x_isa);
    ip2 = (int *)(&x->x_gui.x_fsf);
    binbuf_addv(b, "ssiisiiffiisssiiiiiiiii", gensym("#X"),gensym("obj"),
		(t_int)x->x_gui.x_obj.te_xpix, (t_int)x->x_gui.x_obj.te_ypix,
		atom_getsymbol(binbuf_getvec(x->x_gui.x_obj.te_binbuf)),
        x->x_gui.x_h, x->x_gui.x_h,
		(float)x->x_min, (float)x->x_max,
		x->x_lin0_log1, (*ip1)&IEM_INIT_ARGS_ALL,
		srl[0], srl[1], srl[2],
		x->x_gui.x_ldx, x->x_gui.x_ldy,
		(*ip2)&IEM_FSTYLE_FLAGS_ALL, x->x_gui.x_fontsize,
		bflcol[0], bflcol[1], bflcol[2],
		x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}

void knob_check_height(t_knob *x, int h)
{
    if(h < IEM_SL_MINSIZE)
	h = IEM_SL_MINSIZE;
    x->x_gui.x_h = h;
    if(x->x_val > (x->x_gui.x_h*100 - 100))
    {
	x->x_pos = x->x_gui.x_h*100 - 100;
	x->x_val = x->x_pos;
    }
    if(x->x_lin0_log1)
	x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_h - 1);
    else
	x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

void knob_check_minmax(t_knob *x, double min, double max)
{
    if(x->x_lin0_log1)
    {
	if((min == 0.0)&&(max == 0.0))
	    max = 1.0;
	if(max > 0.0)
	{
	    if(min <= 0.0)
		min = 0.01*max;
	}
	else
	{
	    if(min > 0.0)
		max = 0.01*min;
	}
    }
    x->x_min = min;
    x->x_max = max;
    if(x->x_min > x->x_max)                /* bugfix */
	x->x_gui.x_isa.x_reverse = 1;
    else
        x->x_gui.x_isa.x_reverse = 0;
    if(x->x_lin0_log1)
	x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_h - 1);
    else
	x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void knob_properties(t_gobj *z, t_glist *owner)
{
    t_knob *x = (t_knob *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    
    sprintf(buf, "pdtk_iemgui_dialog %%s KNOB \
	    --------dimensions(pix)(pix):-------- %d %d NONE: %d %d height: \
	    -----------output-range:----------- %g left: %g right: %g \
	    %d lin log %d %d empty %d \
	    %s %s \
	    %s %d %d \
	    %d %d \
	    %d %d %d\n",
	    x->x_gui.x_w, IEM_SL_MINSIZE, x->x_gui.x_h, IEM_GUI_MINSIZE,
	    x->x_min, x->x_max, 0.0,/*no_schedule*/
	    x->x_lin0_log1, x->x_gui.x_isa.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
	    srl[0]->s_name, srl[1]->s_name,
	    srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
	    x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
	    0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void knob_bang(t_knob *x)
{
    double out;
    
    if(x->x_lin0_log1)
	out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
	out = (double)(x->x_val)*0.01*x->x_k + x->x_min;
    if((out < 1.0e-10)&&(out > -1.0e-10))
	out = 0.0;
    //post("knob_bang -- x_k: %f", x->x_k);

    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
	pd_float(x->x_gui.x_snd->s_thing, out);
}

static void knob_dialog(t_knob *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int steady = (int)atom_getintarg(17, argc, argv);
    int sr_flags;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady)
	x->x_steady = 1;
    else
	x->x_steady = 0;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_h = iemgui_clip_size(w);
    knob_check_height(x, h);
    knob_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(x->x_gui.x_glist, (t_text*)x);
}

static void knob_motion(t_knob *x, t_floatarg dx, t_floatarg dy)
{
    int old = x->x_val;

    if(x->x_gui.x_fsf.x_finemoved)
	x->x_pos -= (int)dy;
    else
	x->x_pos -= 100*(int)dy;
    x->x_val = x->x_pos;
    if(x->x_val > (100*x->x_gui.x_h - 100))
    {
	x->x_val = 100*x->x_gui.x_h - 100;
	x->x_pos += 50;
	x->x_pos -= x->x_pos%100;
    }
    if(x->x_val < 0)
    {
	x->x_val = 0;
	x->x_pos -= 50;
	x->x_pos -= x->x_pos%100;
    }
    if(old != x->x_val)
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	knob_bang(x);
    }
}

static void knob_click(t_knob *x, t_floatarg xpos, t_floatarg ypos,
			  t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    if(!x->x_steady)
	x->x_val = (int)(100.0 * (x->x_gui.x_h + text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist) - ypos));
    if(x->x_val > (100*x->x_gui.x_h - 100))
	x->x_val = 100*x->x_gui.x_h - 100;
    if(x->x_val < 0)
	x->x_val = 0;
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    knob_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g, (t_glistmotionfn)knob_motion,
	       0, xpos, ypos);
}

static int knob_newclick(t_gobj *z, struct _glist *glist,
			    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_knob* x = (t_knob *)z;

    if(doit)
    {
	knob_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
		       0, (t_floatarg)alt);
	if(shift)
	    x->x_gui.x_fsf.x_finemoved = 1;
	else
	    x->x_gui.x_fsf.x_finemoved = 0;
    }
    return (1);
}

static void knob_set(t_knob *x, t_floatarg f)
{
    double g;

    if(x->x_gui.x_isa.x_reverse)    /* bugfix */
    {
	if(f > x->x_min)
	    f = x->x_min;
	if(f < x->x_max)
	    f = x->x_max;
    }
    else
    {
	if(f > x->x_max)
	    f = x->x_max;
	if(f < x->x_min)
	    f = x->x_min;
    }
    if(x->x_lin0_log1)
	g = log(f/x->x_min)/x->x_k;
    else
	g = (f - x->x_min) / x->x_k;
    //post("knob_set f: %f", f );
    //post("knob_set g: %f", g );
    x->x_val = (int)(100.0*g + 0.49999);
    // x->x_val = (int)(100.0*g);
    x->x_pos = x->x_val;
    //post("knob_set x_val: %f", x->x_val );
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void knob_float(t_knob *x, t_floatarg f)
{
   /*  post("knob_set infloat f: %f", f ); */
    knob_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
	knob_bang(x);
}

static void knob_size(t_knob *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_h = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
	knob_check_height(x, (int)atom_getintarg(1, ac, av));
    iemgui_size((void *)x, &x->x_gui);
}

static void knob_delta(t_knob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void knob_pos(t_knob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void knob_range(t_knob *x, t_symbol *s, int ac, t_atom *av)
{
    knob_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
			 (double)atom_getfloatarg(1, ac, av));
}

static void knob_color(t_knob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void knob_send(t_knob *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void knob_receive(t_knob *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void knob_label(t_knob *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void knob_label_pos(t_knob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void knob_label_font(t_knob *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void knob_log(t_knob *x)
{
    x->x_lin0_log1 = 1;
    knob_check_minmax(x, x->x_min, x->x_max);
}

static void knob_lin(t_knob *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void knob_init(t_knob *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void knob_steady(t_knob *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

#define LB_LOAD 0 /* from g_canvas.h */

static void knob_loadbang(t_knob *x, t_floatarg action)
{
    if (action == LB_LOAD && x->x_gui.x_isa.x_loadinit)
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	knob_bang(x);
    } 
}

/*
static void knob_list(t_knob *x, t_symbol *s, int ac, t_atom *av)
{
    int l=iemgui_list((void *)x, &x->x_gui, s, ac, av);

    if(l < 0)
    {
	if(IS_A_FLOAT(av,0))
	    knob_float(x, atom_getfloatarg(0, ac, av));
    }
    else if(l > 0)
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
	canvas_fixlinesfor(x->x_gui.x_glist, (t_text*)x);
    }
}
*/

static void *knob_new(t_symbol *s, int argc, t_atom *argv)
{
    t_knob *x = (t_knob *)pd_new(knob_class);
    int bflcol[]={-262144, -1, -1};
    t_symbol *srl[3];
    int w=IEM_KNOB_DEFAULTSIZE, h=IEM_KNOB_DEFAULTSIZE;
    int lilo=0, ldx=0, ldy=-8;
    int fs=8, v=0, steady=1;
    double min=0.0, max=(double)(IEM_SL_DEFAULTSIZE-1);
    char str[144];

    //srl[0] = gensym("empty");
    //srl[1] = gensym("empty");
    //srl[2] = gensym("empty");

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);


    if(((argc == 17)||(argc == 18))&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)
       &&IS_A_FLOAT(argv,14)&&IS_A_FLOAT(argv,15)&&IS_A_FLOAT(argv,16))
   {
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        min = (double)atom_getfloatarg(2, argc, argv);
        max = (double)atom_getfloatarg(3, argc, argv);
        lilo = (int)atom_getintarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = (int)atom_getintarg(9, argc, argv);
        ldy = (int)atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(11, argc, argv));
        fs = (int)atom_getintarg(12, argc, argv);
        bflcol[0] = (int)atom_getintarg(13, argc, argv);
        bflcol[1] = (int)atom_getintarg(14, argc, argv);
        bflcol[2] = (int)atom_getintarg(15, argc, argv);
        v = (int)atom_getintarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);
    if((argc == 18)&&IS_A_FLOAT(argv,17))
        steady = (int)atom_getintarg(17, argc, argv);

    x->x_gui.x_draw = (t_iemfunptr)knob_draw;

    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;

    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if(x->x_gui.x_isa.x_loadinit)
        x->x_val = v;
    else
        x->x_val = 0;
    x->x_pos = x->x_val;
    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady != 0) steady = 1;
    x->x_steady = steady;
    if (!strcmp(x->x_gui.x_snd->s_name, "empty"))
        x->x_gui.x_fsf.x_snd_able = 0;
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
        x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, "courier"); }
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_h = iemgui_clip_size(h);
    knob_check_height(x, w);
    knob_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    //x->x_thick = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return (x);
}



static void knob_free(t_knob *x)
{
    if(x->x_gui.x_fsf.x_selected)
	pd_unbind(&x->x_gui.x_obj.ob_pd, iemgui_key_sym);
    if(x->x_gui.x_fsf.x_rcv_able)
	pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void knob_setup(void)
{
    knob_class = class_new(gensym("knob"), (t_newmethod)knob_new,
			      (t_method)knob_free, sizeof(t_knob), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)knob_new, gensym("knob"), A_GIMME, 0);
    class_addbang(knob_class,knob_bang);
    class_addfloat(knob_class,knob_float);

    /*    class_addlist(knob_class, knob_list); */
    class_addmethod(knob_class, (t_method)knob_click, gensym("click"),
		    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_motion, gensym("motion"),
		    A_FLOAT, A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_dialog, gensym("dialog"),
		    A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_loadbang, gensym("loadbang"),
        A_DEFFLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_size, gensym("size"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_range, gensym("range"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_color, gensym("color"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(knob_class, (t_method)knob_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(knob_class, (t_method)knob_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(knob_class, (t_method)knob_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_log, gensym("log"), 0);
    class_addmethod(knob_class, (t_method)knob_lin, gensym("lin"), 0);
    class_addmethod(knob_class, (t_method)knob_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_steady, gensym("steady"), A_FLOAT, 0);
    if(!iemgui_key_sym)
 	iemgui_key_sym = gensym("#keyname");
    knob_widgetbehavior.w_getrectfn =    knob_getrect;
    knob_widgetbehavior.w_displacefn =   iemgui_displace;
    knob_widgetbehavior.w_selectfn =     iemgui_select;
    knob_widgetbehavior.w_activatefn =   NULL;
    knob_widgetbehavior.w_deletefn =     iemgui_delete;
    knob_widgetbehavior.w_visfn =        iemgui_vis;
    knob_widgetbehavior.w_clickfn =      knob_newclick;
#if PD_MINOR_VERSION < 37 /* TODO: remove old behaviour in exactly 2 months from now */
	knob_widgetbehavior.w_propertiesfn = knob_properties;;
    knob_widgetbehavior.w_savefn =       knob_save;
#else
	class_setpropertiesfn(knob_class, &knob_properties);
	class_setsavefn(knob_class, &knob_save);
#endif
	class_setwidget(knob_class, &knob_widgetbehavior);
    class_sethelpsymbol(knob_class, gensym("knob"));
}
