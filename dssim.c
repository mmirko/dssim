#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <CL/opencl.h>
#include <graphviz/gvc.h>
#include <graphviz/graph.h>
#include <gd.h>
#include <gdfonts.h>

#define MAX_SOURCE_SIZE (0x100000)
#define BORDERPERCENT 2
#define STEPS 10.0
#define ARRFIX 10.0
#define ARRFF 4.0

#define MAX_PLATFORMS 8
#define MAX_DEVICES 64

/* the flag definitions */
#define EXEC_RESET	0x00

#define EXEC_RUN	0x01
#define EXEC_STATE	0x02
#define EXEC_MESSAGE	0x04
/*
			0x08
			0x10
			0x20
			0x40
			0x80
*/

/* macros to manipulate the executuin status */
#define RESET_EXEC(x)		(x = EXEC_RESET)

#define SET_RUN(x)		(x |= EXEC_RUN)
#define SET_STATE(x)		(x |= EXEC_STATE)
#define SET_MESSAGE(x)		(x |= EXEC_MESSAGE)

#define UNSET_RUN(x)		(x &= (~EXEC_RUN))
#define UNSET_STATE(x)		(x &= (~EXEC_STATE))
#define UNSET_MESSAGE(x)	(x &= (~EXEC_MESSAGE))

#define TOGGLE_RUN(x)		(x ^= EXEC_RUN)
#define TOGGLE_STATE(x)		(x ^= EXEC_STATE)
#define TOGGLE_MESSAGE(x)	(x ^= EXEC_MESSAGE)

/* these evaluate to non-zero if the flag is set */
#define IS_RUN(x)		(x & EXEC_RUN)
#define IS_STATE(x)		(x & EXEC_STATE)
#define IS_MESSAGE(x)		(x & EXEC_MESSAGE)

// Restrictions

#define REST_T_INDEX		0
#define REST_T_VALUE		0x01
#define REST_T_SET(x)		(x[REST_T_INDEX] |= REST_T_VALUE)
#define REST_T_CHECK(x)		(x[REST_T_INDEX] & REST_T_VALUE)

#include "transformer.c"
#include "messages.c"

//////////////////////////////////////// In case of custom kernels you have to config these:
#define REGISTERS 1
#define MESSTYPES 1

void defaults(int * states, int * messages, int nodes, int step)
{
	int i,j,k;

	// Step -1 states and initial messages, step 0 messages_out.

	if (step==-1) {
		for (i=0;i<nodes;i++) {
			// States defaults
			for (j=0 ; j < REGISTERS ; j++) {
				*(states+i*REGISTERS+j)=0;
			}
		}

		for (i=0;i<nodes;i++) {
			for (j=0;j<nodes;j++) {
				// Messages defaults (both steps)
				for (k=0 ; k < MESSTYPES ; k++) {
					*(messages+(i*nodes+j)*MESSTYPES+k)=0;
				}
			}
		}
	} else {
		// Messages default (step 0)
		*(messages)=1;
		*(states)=1;
	}
}

void ending(int * ending_states, int * ending_messages, int registers, int messtypes)
{
	*(ending_messages)=0;
	*(ending_messages+1)=0;
	*(ending_states)=2;
}

/////////////////////////////////////////////////////////////////////////////////////////////

struct style_list {
	char * attr;
	char * value;
	struct style_list * next;
};

struct style_entry {
	char * name;
	struct style_list * top;
	struct style_entry * next;
};

struct teddata {
	int i;
	int j;
	int k;
	struct teddata * next;
	struct teddata * old;
};

void lua_defaults(lua_State *L,Agnode_t **  ithnode,  int * states, int * messages, int nodes, int step, int registers, int messtypes, int * message_defaults)
{
	int i,j,k;
	int tempdefault;

	if (step==-1) {
		for (j=0;j<registers;j++) {
			lua_getglobal(L, "get_default_state");
			lua_pushinteger(L,j);
			lua_call(L,1,1);
			if (lua_isnil(L,-1)) {
				fprintf (stderr,"Missing default.");
				lua_close(L);
				exit(1);
			}
			tempdefault=lua_tointeger(L, -1);
			lua_pop(L,1);
	
			for (i=0;i<nodes;i++) {
				*(states+i*registers+j)=tempdefault;
			}
		}
	
		for (k=0;k<messtypes;k++) {
			lua_getglobal(L, "get_default_mess");
			lua_pushinteger(L,k);
			lua_call(L,1,1);
			if (lua_isnil(L,-1)) {
				fprintf (stderr,"Missing default.");
				lua_close(L);
				exit(1);
			}
			tempdefault=lua_tointeger(L, -1);
			lua_pop(L,1);

			*(message_defaults+k)=tempdefault;
	
			for (i=0;i<nodes;i++) {
				for (j=0;j<nodes;j++) {
					*(messages+(i*nodes+j)*messtypes+k)=tempdefault;
				}
			}
		}
	} else {
		int l;

		// Number of entities that have modifications
		int bnum;

		// Temporary variables that store the entity id and name
		int bid;
		char * bname;

		// Get the number of node modified in the temporal step 
		lua_getglobal(L, "get_boundary_num");
		lua_pushinteger(L,step);
		lua_call(L,1,1);
		if (!lua_isnil(L,-1)) {
			bnum=lua_tointeger(L, -1);
			lua_pop(L,1);
		} else {
			lua_pop(L,1);
			return;
		}
		for (l=0;l<bnum;l++) {
			// Getting the node name that has modification
			lua_getglobal(L, "get_boundary_el_name");
			lua_pushinteger(L,step);
			lua_pushinteger(L,l);
			lua_call(L,2,1);
			if (!lua_isnil(L,-1)) {
				bname=(char *) lua_tostring(L, -1);
				bid=search_node_id_from_name(ithnode,bname,nodes);

				if (bid==-1) {
					fprintf (stderr,"node %s in the init file does not exist in graph",bname);
					lua_close(L);
					exit(1);
				}

				for (j=0;j<registers;j++) {
					lua_getglobal(L, "get_boundary_el_state");
					lua_pushinteger(L,step);
					lua_pushstring(L,bname);
					lua_pushinteger(L,j);
					lua_call(L,3,1);
					if (!lua_isnil(L,-1)) {
						tempdefault=lua_tointeger(L, -1);
						*(states+bid*registers+j)=tempdefault;
					}
					lua_pop(L,1);
				}

				for (k=0;k<messtypes;k++) {
					lua_getglobal(L, "get_boundary_el_mess");
					lua_pushinteger(L,step);
					lua_pushstring(L,bname);
					lua_pushinteger(L,k);
					lua_call(L,3,1);
					if (!lua_isnil(L,-1)) {
						tempdefault=lua_tointeger(L, -1);
						*(messages+(bid*nodes+bid)*messtypes+k)=tempdefault;
					}
					lua_pop(L,1);
				}
				lua_pop(L,1);
			} else {
				lua_pop(L,1);
				return;
			}

		}
	}
}

void lua_ending(lua_State *L, int * ending_states, int * ending_messages, int registers, int messtypes)
{
	int i,j,k;
	int tempdefault;

	for (j=0;j<registers;j++) {
		lua_getglobal(L, "get_ending_state");
		lua_pushinteger(L,j);
		lua_call(L,1,1);
		if (lua_isnil(L,-1)) {
			fprintf (stderr,"Missing ending state.");
			lua_close(L);
			exit(1);
		}
		tempdefault=lua_tointeger(L, -1);
		lua_pop(L,1);

		*(ending_states+j)=tempdefault;
	}

	for (k=0;k<messtypes;k++) {
		lua_getglobal(L, "get_ending_mess");
		lua_pushinteger(L,k);
		lua_call(L,1,1);
		if (lua_isnil(L,-1)) {
			fprintf (stderr,"Missing ending message.");
			lua_close(L);
			exit(1);
		}
		tempdefault=lua_tointeger(L, -1);
		lua_pop(L,1);

		*(ending_messages+k)=tempdefault;
	}
}

int lua_check_report(lua_State *L, char * report_name)
{
	lua_getglobal(L, "check_report");
	lua_pushstring(L,report_name);
	lua_call(L,1,1);
	if (lua_isnil(L,-1)) {
		lua_pop(L,1);
		return 0;
	} else {
		lua_pop(L,1);
		return 1;
	}
}

// Id to name resolution (in case of custom opmode the resolution do not work)
char * id_to_name(lua_State *L,int opmode,int regotmess, int lev1, int lev2)
{
	char * result;

	// The resolution does not work in opmode 1
	if ((opmode == 1)||(L== NULL)) {
		return NULL;
	}

	lua_getglobal(L, "id_to_name");

	// 0 is a register
	if (regotmess==0) {
		lua_pushstring(L,"register");	
	} else {
		lua_pushstring(L,"mess");	
	}

	lua_pushinteger(L,lev1);

	// lev2 == -10 means to resolve the name of the level1
	if (lev2 == -10 ) {
		lua_pushnil(L);
	} else {
		lua_pushinteger(L,lev2);
	}

	lua_call(L,3,1);
	if (lua_isnil(L,-1)) {
		fprintf (stderr,"Wrong name resolution.");
		lua_close(L);
		exit(1);
	}

	result = (char *) strdup(lua_tostring(L,-1));
	lua_pop(L,1);

	return result;
}


struct style_entry * lua_styles(lua_State *L)
{
	struct style_entry * styles=NULL;
	struct style_entry * istyles;
	struct style_entry * jstyles;
	struct style_list * ilist;
	struct style_list * jlist;


	lua_getglobal(L, "styles");
	if (!lua_isnil(L,-1)) {
		lua_pushnil(L);  /* first key */
		while (lua_next(L, -2) != 0) {

			istyles=(struct style_entry *) malloc(sizeof(struct style_entry));
			istyles->name=strdup(lua_tostring(L, -2));
			istyles->next=NULL;
			istyles->top=NULL;

			// Registering the key - value pairs for the given register or message
			lua_pushvalue(L,-1);
			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {

				ilist=(struct style_list *) malloc(sizeof(struct style_list));
				ilist->attr=strdup(lua_tostring(L, -2));
				ilist->value=strdup(lua_tostring(L, -1));
				ilist->next=NULL;

				if (istyles->top==NULL) {
					istyles->top=ilist;
				} else {
					jlist->next=ilist;
				}

				jlist=ilist;

				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			if (styles==NULL) {
				styles=istyles;
			} else {
				jstyles->next=istyles;
			}

			jstyles=istyles;

			/* removes 'value'; keeps 'key' for next iteration */
			lua_pop(L, 1);
		}
	}

	lua_pop(L,1);
	return styles;
}

void styles_print(struct style_entry * styles)
{
	struct style_entry * istyles;
	struct style_list * ilist;

	for (istyles=styles;istyles!=NULL;istyles=istyles->next) {
		printf ("   %s\n",istyles->name);
		for (ilist=istyles->top;ilist!=NULL;ilist=ilist->next) {
			printf ("      %s -> %s\n",ilist->attr,ilist->value);
		}
	}
}


// The node styling function
void style_node(lua_State *L,int opmode, struct style_entry * styles, Agnode_t **  ithnode, int i, int * states, int nodes, int registers)
{
	int j;
	int hit;
	char * tempstr;
	struct style_entry * istyles;
	struct style_list * ilist;

	for (j=0, hit=0; j < registers ; j++) {
		tempstr=(char *) id_to_name(L,opmode,0,j,*(states+i*registers+j));

		for (istyles=styles;istyles!=NULL;istyles=istyles->next) {
			if (!strcmp(tempstr,istyles->name)) {
				for (ilist=istyles->top;ilist!=NULL;ilist=ilist->next) {
					agsafeset(*(ithnode+i), ilist->attr, ilist->value, "");
				}
				hit=1;
			}
		}

		free(tempstr);
	}

	if (hit==0) {
		for (j=0 ; j < registers ; j++) {
			for (istyles=styles,hit=0;istyles!=NULL;istyles=istyles->next) {
				if (!strcmp("registers_default",istyles->name)) {
					for (ilist=istyles->top;ilist!=NULL;ilist=ilist->next) {
						agsafeset(*(ithnode+i), ilist->attr, ilist->value, "");
					}
				}
			}
		}
	}
}

// The edge styling function
void style_edge(lua_State *L,int opmode, struct style_entry * styles, Agraph_t * dsgraph, Agnode_t ** ithnode, int i, int j, int * messages, int nodes, int messtypes)
{
	int k;
	int hit;
	char * tempstr;
	Agedge_t * iedge;
	Agedge_t * iedge2;
	struct style_entry * istyles;
	struct style_list * ilist;

	iedge=agfindedge(dsgraph,*(ithnode+i),*(ithnode+j));

	for (k=0, hit=0; k < messtypes ; k++) {

		tempstr=(char *) id_to_name(L,opmode,1,k,*(messages+(i*nodes+j)*messtypes+k));

		for (istyles=styles;istyles!=NULL;istyles=istyles->next) {
			if (!strcmp(tempstr,istyles->name)) {
				for (ilist=istyles->top;ilist!=NULL;ilist=ilist->next) {
					agsafeset(iedge, ilist->attr, ilist->value, "");
				}
				hit=1;
			}
		}

		free(tempstr);
	}

	if (hit==0) {
		for (k=0 ; k < messtypes ; k++) {
			for (istyles=styles,hit=0;istyles!=NULL;istyles=istyles->next) {
				if (!strcmp("messages_default",istyles->name)) {
					for (ilist=istyles->top;ilist!=NULL;ilist=ilist->next) {
						agsafeset(iedge, ilist->attr, ilist->value, "");
					}
				}
			}
		}
	}
						
}


int check_end(int * states, int * messages, int nodes, int * ending_states, int * ending_messages, int registers, int messtypes)
{
	int i,j,k;
	for (j=0;j<registers;j++) {
		for (i=0;i<nodes;i++) {
			if (*(states+i*registers+j)!=*(ending_states+j)) return 0;
		}
	}
	for (i=0;i<nodes;i++) {
		for (j=0;j<nodes;j++) {
			for (k=0;k<messtypes;k++) {
				if (*(messages+(i*nodes+j)*messtypes+k)!=*(ending_messages+k)) return 0;
			}
		}
	}
	return 1;
}

void version()
{
	printf("DSSim - Distributed System OpenCL Simulator\nCopyright 2013 - Mirko Mariotti - http://www.mirkomariotti.it\n");
	fflush(stdout);
}

void usage()
{
	printf("DSSim - Distributed System OpenCL Simulator\nCopyright 2013 - Mirko Mariotti - http://www.mirkomariotti.ii\nUsage:\n\n");
	printf("\tdssim -g graph_dot_file -p protocol_file -i init_file [-s OpenCL_custom_kernel] [-t time] [-v] [-o] [-T type] [-b] [-e]\n");
	printf("\t(expert only) dssim -g graph_dot_file -k OpenCL_custom_protocol_file [-v][-o]\n");
	printf("\tdssim -V\n\n");
	printf("\tOptions:\n");
	printf("\t\t-h       - This help\n");
	printf("\t\t-v       - Be verbose\n");
	printf("\t\t-V       - Print program version and exit\n");
	printf("\t\t-k file  - Select the OpenCL custom kernel file (experts only)\n");
	printf("\t\t-p file  - Select the protocol file\n");
	printf("\t\t-g file  - Select the graph description file (graphviz dot file)\n");
	printf("\t\t-i file  - Select the initialization file\n");
	printf("\t\t-s file  - Save the created kernel as file\n");
	printf("\t\t-t time  - Set the simulation time (default 1000)\n");
	printf("\t\t-o       - Generate a PNG with a graph for each simulation step\n");
	printf("\t\t-e       - Generate a PNG with a TED chart for each simulation step\n");
	printf("\t\t-l       - List OpenCL platforms\n");
	printf("\t\t-L       - List OpenCL devices\n");
	printf("\t\t-d id    - Select OpenCL device with the given id\n");
	printf("\t\t-T type  - Select Graphviz rendering: \"dot\", \"neato\" or \"gtk\"\n");
	printf("\t\t-b       - Bypass the check of ending condition\n");
	fflush(stdout);

}

// Shutdown on errors
void shutdown(GVC_t* gvc, graph_t * dsgraph, lua_State *L)
{
	agclose(dsgraph);
	gvFreeContext(gvc);
	lua_close(L);
	exit(1);
}

// Search the id of the node given its host address
int search_node_id(Agnode_t ** ithnode, Agnode_t * inode,int nodes)
{
	int i;
	for (i=0;i<nodes;i++) {
		if (inode==*(ithnode+i)) {
			return i;
		}
	}
	return -1;
}

// Search the id of the node given its name
int search_node_id_from_name(Agnode_t ** ithnode, char * name,int nodes)
{
	int i;

	for (i=0;i<nodes;i++) {
		if (!strcmp(name,(*(ithnode+i))->name)) {
			return i;
		}
	}
	return -1;
}

void default_layout(gdImagePtr * tedim, int tedimx, int tedimy)
{
	int border;
	int white;
	int bordper=BORDERPERCENT;

	border = (int) ((bordper*tedimx)/100);
	if (border > (bordper*tedimy)/100) border=(bordper*tedimy)/100;

	white = gdImageColorAllocate(*tedim,255,255,255);

	gdImageLine(*tedim,border,border,tedimx-border,border,white);
	gdImageLine(*tedim,tedimx-border,border,tedimx-border,tedimy-border,white);
	gdImageLine(*tedim,tedimx-border,tedimy-border,border,tedimy-border,white);
	gdImageLine(*tedim,border,border,border,tedimy-border,white);
}

int nodes_layout(gdImagePtr * tedim, int tedimx, int tedimy, Agnode_t ** ithnode,int nodes)
{
	int i;

	int border;
	int white;
	int bordper=BORDERPERCENT;
	int step;
	int avspace;
	int maxlen;

	border = (int) ((bordper*tedimx)/100);
	if (border > (bordper*tedimy)/100) border=(bordper*tedimy)/100;

	white = gdImageColorAllocate(*tedim,255,255,255);

	avspace=tedimy-4*border;
	step=avspace/(nodes);

	maxlen=0;
	for (i=0;i<nodes;i++) {
		if (strlen((*(ithnode+i))->name) > maxlen) {
			maxlen=strlen((*(ithnode+i))->name);
		}
	}

	for (i=0;i<nodes;i++) {
		gdImageString(*tedim, gdFontGetSmall(),2*border,(2*border+i*step)- gdFontGetSmall()->h / 2 ,(*(ithnode+i))->name, white);
		gdImageLine(*tedim,3*border + maxlen * gdFontGetSmall()->w, (2*border+i*step) ,tedimx-2*border,(2*border+i*step) ,white);
	}

	return 3*border + maxlen * gdFontGetSmall()->w;
}

void step_layout(gdImagePtr * tedim, int tedimx, int tedimy, int * messages, int * message_defaults, int nodes, int messtypes,int step,struct teddata ** teddatas, int startdrawx)
{
	int i,j,k,ii;

	int border;
	int white,red,green;
	int bordper=BORDERPERCENT;
	float stepx,stepy,startx,starty,x3,y3,x4,y4,x5,y5;
	int avspace;
	char temps[5];

	gdPoint points[3];

	struct teddata * curtteddatas=NULL;
	struct teddata * iteddatas=NULL;
	struct teddata * jteddatas=NULL;

	float delta;

	border = (int) ((bordper*tedimx)/100);
	if (border > (bordper*tedimy)/100) border=(bordper*tedimy)/100;

	white = gdImageColorAllocate(*tedim,255,255,255);
	red = gdImageColorAllocate(*tedim,255,0,0);
	green = gdImageColorAllocate(*tedim,0,255,0);

	avspace=tedimy-4*border;
	stepx=(tedimx-2*border-startdrawx)/STEPS;
	stepy=avspace/(nodes);

	startx=startdrawx;
	starty=2*border;

	int stepmax=STEPS;
	int startstep=0;
	if (step>stepmax) {
		startstep=step-stepmax;
		stepmax=step;
	}

	for (;step>STEPS;step--) {}

	for (i=startstep;i<=stepmax;i++) {
		sprintf(temps,"t=%d",i+1);
		gdImageDashedLine(*tedim,startx+(i-startstep)*stepx , starty ,startx+(i-startstep)*stepx, tedimy-starty ,red);
		if (i!=stepmax) gdImageString(*tedim, gdFontGetSmall(), startx+(i-startstep)*stepx + border, 2*border + stepy*nodes, temps , red);
	}

	gdImageSetAntiAliased(*tedim, green);

	for (iteddatas=(*teddatas),ii=1;(ii<STEPS)&&(iteddatas!=NULL);ii++,iteddatas=iteddatas->old) {
		for (jteddatas=iteddatas;jteddatas!=NULL;jteddatas=jteddatas->next) {
			i=jteddatas->i;
			j=jteddatas->j;
			k=jteddatas->k;

			if (i!=-1) {

				gdImageLine(*tedim,startx+(step-1-ii)*stepx , starty+i*stepy ,startx+(step-ii)*stepx,  starty+j*stepy ,gdAntiAliased);

				delta=sqrt((j-i)*(j-i)*stepy*stepy+stepx*stepx);
				x3=startx+(step-ii)*stepx-ARRFIX*stepx/delta;
				y3=starty+j*stepy-ARRFIX*(j-i)*stepy/delta;
				x4=startx+(step-ii)*stepx-((ARRFIX*(stepx+((j-i)*stepy)/ARRFF))/delta);
				x5=startx+(step-ii)*stepx-((ARRFIX*(stepx-((j-i)*stepy)/ARRFF))/delta);
				y4=starty+j*stepy-((ARRFIX*((j-i)*stepy-stepx/ARRFF))/delta);
				y5=starty+j*stepy-((ARRFIX*((j-i)*stepy+stepx/ARRFF))/delta);
				points[0].x = (int) startx+(step-ii)*stepx;
				points[0].y = (int) starty+j*stepy;
				points[1].x = (int) x4;
				points[1].y = (int) y4;
				points[2].x = (int) x5;
				points[2].y = (int) y5;
				gdImageFilledPolygon(*tedim, points, 3, green);
			}
		}
	}

	for (i=0;i<nodes;i++) {
		for (j=0;j<nodes;j++) {
			for (k=0 ; k < messtypes ; k++) {
				if ( *(messages+(i*nodes+j)*messtypes+k)!=*(message_defaults+k)) {
					gdImageLine(*tedim,startx+(step-1)*stepx , starty+i*stepy ,startx+(step)*stepx,  starty+j*stepy ,gdAntiAliased);

					delta=sqrt((j-i)*(j-i)*stepy*stepy+stepx*stepx);
					x3=startx+(step)*stepx-ARRFIX*stepx/delta;
					y3=starty+j*stepy-ARRFIX*(j-i)*stepy/delta;
					x4=startx+step*stepx-((ARRFIX*(stepx+((j-i)*stepy)/ARRFF))/delta);
					x5=startx+step*stepx-((ARRFIX*(stepx-((j-i)*stepy)/ARRFF))/delta);
					y4=starty+j*stepy-((ARRFIX*((j-i)*stepy-stepx/ARRFF))/delta);
					y5=starty+j*stepy-((ARRFIX*((j-i)*stepy+stepx/ARRFF))/delta);
					points[0].x = (int) startx+(step)*stepx;
					points[0].y = (int) starty+j*stepy;
					points[1].x = (int) x4;
					points[1].y = (int) y4;
					points[2].x = (int) x5;
					points[2].y = (int) y5;
					gdImageFilledPolygon(*tedim, points, 3, green);

					iteddatas=(struct teddata *) malloc(sizeof(struct teddata));
					iteddatas->i=i;
					iteddatas->j=j;
					iteddatas->k=k;
					iteddatas->next=NULL;
					iteddatas->old=NULL;

					if (curtteddatas==NULL) {
						curtteddatas=iteddatas;
					} else {
						iteddatas->next=curtteddatas;
						curtteddatas=iteddatas;
					}
				}
			}
		}
	}

	if (curtteddatas==NULL) {
		curtteddatas=(struct teddata *) malloc(sizeof(struct teddata));
		curtteddatas->i=-1;
		curtteddatas->j=-1;
		curtteddatas->k=-1;
		curtteddatas->next=NULL;
		curtteddatas->old=NULL;
	}

	if (*teddatas == NULL) {
		*teddatas=curtteddatas;
	} else {
		curtteddatas->old=(*teddatas);
		*teddatas=curtteddatas;
	}
}

int LCM(int n,int m) {
	int lcm,h;
	h=GCD(n,m);

	lcm=(n*m)/(h);
	return lcm;
}
     
int GCD(int n, int m) {
	int c;
	if(n<m) {
		c=n; n=m; m=c;
	}
	
	while (m != 0) {
		int r=n%m;
		n=m;
		m=r;
	}
	return n;
}

int main( int argc, char* argv[] )
{
	// Temp string
	char * tempstr;

	// Counters
	int c,index;
	int i,j,k,l;

	// Time
	int timetick;

	// Restrictions;
	unsigned char restrictions[] = {0,0};
	char * rest_name;
	int rest_num=0;

	// The protocol filename
	char * protocol_file = NULL;

	// The graph filename
	char * graph_file = NULL;

	// The custom kernel filename
	char * kernel_file = NULL;

	// The initial condition filename
	char * initial_file = NULL;

	// Save kernel file name
	char * save_kernel = NULL;

	// Output target
	char * output_target = NULL;

	// Graph(viz) managment
	GVC_t* gvc;
	Agraph_t * dsgraph;

	Agnode_t * inode;
	Agedge_t * iedge;

	// Styles
	struct style_entry * styles;

	// Lua interpreter
	lua_State *L = NULL;

	// File counter
	FILE *fp;

	// Verbose flag
	char verbose=0;

	// Node numbers
	unsigned int nodes=0;

	// Node index to Node structure
	Agnode_t ** ithnode;

	// Entity relative speed and next execution array
	int * nspeeds;
	int * ex_next;
	int * ex_incr;
	int * ex_stat;
	int speed_lcm;
	int speed_gcd;

	// Links relative speed and next availybility array
	int * lspeeds;
	int * link_incr;

 	// The links matrix is a nodes x nodes which stores 0 if there in not a link x->y, !=0 otherwise (in the future it may contain same sort of link weight)
	int * links;

	// Message queues
	struct mes_queue * queues;

	// Node registers
	unsigned int registers = REGISTERS;

	// Message types
	unsigned int messtypes = MESSTYPES;

	// Array fot the default message value
	int *message_defaults;

 	// The states matrix is a nodes x registers which stores for each entity the value of the given register. Two things:
 	// 1 - The register value if up to the opencl kernel, it is relevant to the host only for analysis purposes.
 	// 2 - The first register is the state register and has to exist.
	int *states;

 	// The messages matrix is a nodes x nodes x messtypes which stores for each x to y comunication if the message of type z has arrived
	int *messages;

	// Ending conditions
	int * ending_states;
	int * ending_messages;

	// Entity to load
	char * entity;
	char * entityfile;

	// OpenCL kernel source variables
	char *kernelSource;
	size_t source_size;

	// Operation mode for initfile: 0 config, 1 custom
	int opmode;

	// Output png files: 0 no, 1 yes
	int pngout=0;

	// Simulation time
	int sim_time=1000;

	// Bypass the ending check
	int bypass_ending=0;

	// Show platforms
	int show_platforms=0;

	// Show devices
	int show_devices=0;

	// Choosen device
	int choosen_device=0;
	int device_exists=0;

	// Generate a ted file
	int ted=0;
	gdImagePtr tedim;
	FILE * tedout;
	int tedimx, tedimy;
	struct teddata * teddatas=NULL;

	///// Program start

	// Size, in bytes, of each vector
	size_t bytes = sizeof(int);

	// Start with the command line parsing
	while ((c = getopt (argc, argv, "hvbVk:p:g:i:s:ot:T:elLd:")) != -1)
	switch (c) {
		case 'h':
			usage();
			exit(0);
			break;
		case 'v':
			verbose=1;
			break;
		case 'V':
			version();
			exit(0);	
			break;
		case 'k':
			kernel_file=strdup(optarg);
			break;
		case 'p':
			protocol_file=strdup(optarg);
			break;
		case 'g':
			graph_file=strdup(optarg);
			break;
		case 'i':
			initial_file=strdup(optarg);
			break;
		case 'T':
			output_target=strdup(optarg);
			break;
		case 's':
			save_kernel=strdup(optarg);
			break;
		case 't':
			sim_time=atoi(optarg);
			break;
		case 'd':
			choosen_device=atoi(optarg);
			break;
		case 'o':
			pngout=1;
			break;
		case 'b':
			bypass_ending=1;
			break;
		case 'l':
			show_platforms=1;
			break;
		case 'L':
			show_devices=1;
			break;
		case 'e':
			ted=1;
			break;
                case '?':
                        if ((optopt == 'k')||(optopt == 'p')||(optopt == 'g')||(optopt == 'i'))
                        {
                                fprintf (stderr, "Option -%c requires an argument.\n\n", optopt);
				usage();
                                exit(1);
                        }
                        else
                        {
                                if (isprint (optopt))
                                {
                                        fprintf (stderr, "Unknown option `-%c'.\n\n", optopt);
					usage();
                                        exit(1);
                                }
                                else
                                {
                                        fprintf (stderr,"Unknown option character `\\x%x'.\n\n",optopt);
					usage();
                                        exit(1);
                                }
                        }
                default:
                        return 1;
	}

	// Exits on wrong options
	for (index = optind; index < argc; index++) {
		fprintf (stderr,"Non-option argument %s\n\n", argv[index]);
		usage();
		exit(1);
	}

	if (verbose) {
		version();
		printf("\n-----\n\n");
	}

	// Open LUA interpreter and libs
	L = luaL_newstate();
	luaL_openlibs(L);

	// Open graphviz context
	gvc = gvContext();

	// Check and process the graph file
	if (graph_file!=NULL) {
		fp=fopen(graph_file,"r");
		if (!fp) {
			fprintf (stderr,"Invalid graph file.\n");
			exit(0);
		}

		aginit();

		dsgraph=agread(fp);

		if (verbose) printf("Importing graph nodes:\n");

		nodes=agnnodes(dsgraph);

		// Allocate memory for ithnode
		ithnode = (Agnode_t **) malloc(nodes*sizeof(Agnode_t *));

		// Allocate memory for node speeds and ex_next and ex_incr and ex_stat
		nspeeds = (int *) malloc(nodes*sizeof(int));
		ex_next = (int *) malloc(nodes*sizeof(int));
		ex_incr = (int *) malloc(nodes*sizeof(int));
		ex_stat = (int *) malloc(nodes*sizeof(int));

		// Allocate memory for link speeds and link_incr
		lspeeds = (int *) malloc(nodes*nodes*sizeof(int));
		link_incr = (int *) malloc(nodes*nodes*sizeof(int));

		// Allocate memory for the link matrix and initialize it and initialize the messages queue and the lspeeds
		links = (int*)malloc(nodes*nodes*bytes);
		queues = (struct mes_queue *) malloc(nodes*nodes*sizeof(struct mes_queue));
		for (i=0;i<nodes*nodes;i++) {
			*(links+i)=0;
			mes_queue_init(queues+i);
			*(lspeeds+i)=0;
			*(link_incr+i)=0;
		}

		// LCM of the speed array
		speed_lcm=1;

		// Populate the id->node resolution
		for (inode=agfstnode(dsgraph),i=0;inode!=NULL;inode=agnxtnode(dsgraph,inode),i++) {
			if (verbose) printf(" - Importing node - %s (Host %p)",inode->name,inode);
			*(ithnode+i)=inode;

			// Get the node relative speed or eventually 1
			tempstr=agget(inode,"rspeed");
			if ((tempstr!=NULL)&&(strcmp("",tempstr))) {
				*(nspeeds+i)=atoi(tempstr);
			} else {
				*(nspeeds+i)=1;
			}
			if (verbose) printf(" - Relative speed %d\n",*(nspeeds+i));

			// Every node compute at time 1
			*(ex_next+i)=1;
			RESET_EXEC(*(ex_stat+i));

			// Computung the LCM of the speed array
			speed_lcm=LCM(speed_lcm,*(nspeeds+i));

			if (i==0) {
				speed_gcd=*(nspeeds+i);
			} else {
				speed_gcd=GCD(speed_gcd,*(nspeeds+i));
			}
		}

		if (verbose) printf("%d nodes imported\n\n",nodes);

		if (verbose) printf("Importing graph edges:\n");

		for (inode=agfstnode(dsgraph);inode!=NULL;inode=agnxtnode(dsgraph,inode)) {
			if (verbose) printf(" - Importing node %s edges:\n",inode->name);
			for (iedge=agfstout(dsgraph,inode);iedge!=NULL;iedge=agnxtout(dsgraph,iedge)) {
				// Find the two indexes
				j=search_node_id(ithnode,iedge->tail,nodes);
				k=search_node_id(ithnode,iedge->head,nodes);

				tempstr=agget(iedge,"index");
				if ((tempstr!=NULL)&&(strcmp("",tempstr))) {
					*(links+j*nodes+k)=atoi(tempstr);
				} else {
					*(links+j*nodes+k)=1;
				}
				if (verbose) printf("   - Importing edge (Label %d) - %s (Host %p) -> %s (Host %p)",*(links+j*nodes+k),iedge->tail->name,iedge->tail,iedge->head->name,iedge->head);

				// Get the relative link speed or eventually 1
				tempstr=agget(iedge,"rspeed");
				if ((tempstr!=NULL)&&(strcmp("",tempstr))) {
					*(lspeeds+j*nodes+k)=atoi(tempstr);
				} else {
					*(lspeeds+j*nodes+k)=1;
				}
				if (verbose) printf(" - Relative speed %d\n",*(lspeeds+j*nodes+k));

				// Computung the LCM and the GCD of the speed array
				speed_lcm=LCM(speed_lcm,*(lspeeds+j*nodes+k));
				speed_gcd=GCD(speed_gcd,*(lspeeds+j*nodes+k));
			}
		}

		if (verbose) printf("%d edges imported\n\n",agnedges(dsgraph));

		if (verbose) {
			printf("Links matrix:\n");
			for (i=0;i<nodes;i++) {
				printf(" ");
				for (j=0;j<nodes;j++) {
					printf("%d ",*(links+i*nodes+j));
				}
				printf("\n");
			}
			printf("\n");
		}

	} else {
		fprintf (stderr,"No graph given, you need to use the -g option.\n\n");
		usage();
		exit(1);
	}

	// Check the operation mode
	if ((protocol_file==NULL)&&(kernel_file==NULL)) {
		fprintf (stderr,"Either a protocol file or a custom opencl kernel is required.\n\n");
		usage();
		exit(1);
	} else if ((protocol_file!=NULL)&&(kernel_file!=NULL)) {
		fprintf (stderr,"A protocol file or a custom opencl kernel is required (not both).\n\n");
		usage();
		exit(1);
	} else if ((protocol_file!=NULL)&&(kernel_file==NULL)) {

		if (initial_file == NULL) {
			fprintf (stderr,"An intial file is needed.\n\n");
			usage();
			exit(1);
		}

		opmode=0;
		
//printf("Transformer function\n");
//printf("%s",transformer_function);
//printf("\n----\n");

		// Load the lua trasformation engine
		if (luaL_dostring(L, transformer_function)) {
			fprintf (stderr,"The transformer lua core did not load.");
			exit(1);
		}
		if (luaL_dofile(L, protocol_file)) {
			fprintf (stderr,"Failed to load the protocol file.");
			exit(1);
		}
		if (luaL_dofile(L, initial_file)) {
			fprintf (stderr,"Failed to load the initial file.");
			exit(1);
		}


		// Get the number of restrictions
		lua_getglobal(L, "get_restrictions_num");
		lua_call(L,0,1);
		rest_num=lua_tointeger(L, -1);
		lua_pop(L,1);

		for (l=0;l<rest_num;l++) {
			// Getting the restriction name
			lua_getglobal(L, "get_restriction_name");
			lua_pushinteger(L,l);
			lua_call(L,1,1);
			rest_name=(char *) lua_tostring(L, -1);
			if (!strcmp(rest_name,"T")) {
				REST_T_SET(restrictions);
			}
			lua_pop(L,1);
		}

		// Find out the number of protocol registers
		lua_getglobal(L, "num_registers");
		lua_call(L,0,1);
		registers=lua_tointeger(L, -1);
		lua_pop(L,1);

		if (registers==0) {
			fprintf (stderr,"The protocol seems to have 0 registers, this cannot be right.");
			shutdown(gvc,dsgraph,L);
		}

		// Find out the number of messages types
		lua_getglobal(L, "num_messtypes");
		lua_call(L,0,1);
		messtypes=lua_tointeger(L, -1);
		lua_pop(L,1);

		if (messtypes==0) {
			fprintf (stderr,"The protocol seems to have 0 messtypes, this cannot be right.");
			shutdown(gvc,dsgraph,L);
		}

		// Allocate memory for messa_defaults
		message_defaults=(int*) malloc(messtypes*sizeof(int));

		// Allocate memory for each vector on host
		states = (int*)malloc(nodes*registers*bytes);
		messages = (int*)malloc(nodes*nodes*messtypes*bytes);

		// Ending states and messages memory
		ending_states = (int*)malloc(registers*bytes);
		ending_messages = (int*)malloc(messtypes*bytes);

		// Populate the ending conditions
		if (bypass_ending==0) {
			lua_ending(L,ending_states,ending_messages,registers,messtypes);	
		}

		// Populate the styles
		styles=lua_styles(L);
		if (verbose) {
			printf("Loading graphical styles:\n");
			styles_print(styles);
		}

		// Find out the number of messages types
		lua_getglobal(L, "transformer");
		lua_pushstring(L,protocol_file);
		lua_call(L,1,2);

		if (lua_isnil(L,-1)) {
			fprintf(stderr, "Transformer has failed with the following message: %s\n",lua_tostring(L, -2));
			shutdown(gvc,dsgraph,L);
		}

		kernelSource=strdup(lua_tostring(L, -2));
		lua_pop(L,1);
		lua_pop(L,1);
		entity=protocol_file;


		if (verbose) {
			printf("Produced OpenCL kernel:\n\n");
			printf("%s",kernelSource);
			printf("\n----\n");
		}

// TODO: Include controls on save kernel

		if (save_kernel != NULL) {
			fp = fopen(save_kernel, "w");
			fprintf(fp,"%s",kernelSource);
			fclose(fp);
		}

	} else if ((protocol_file==NULL)&&(kernel_file!=NULL)) {
		opmode=1;
		entityfile=kernel_file;
		entity= (char *) malloc((int) (strlen(entityfile) - 2)*sizeof(char));
		strncpy(entity,entityfile,(int) strlen(entityfile) - 3);
		*(entity+ strlen(entityfile) - 3) = 0;

		if (verbose) printf("Loading custom OpenCL kernel as protocol:\n   Protocol file: %s\n   Entity OpenCL function: %s\n\n",entityfile,entity);

		// Load the kernel source code into the array kernelSource
		fp = fopen(entityfile, "r");
		if (!fp)
		{
			fprintf(stderr, "Failed to load the kernel!\n");
			return EXIT_FAILURE;
		}
		kernelSource = (char*)malloc(MAX_SOURCE_SIZE);
		source_size = fread( kernelSource, 1, MAX_SOURCE_SIZE, fp);
		fclose(fp);

		// Allocate memory for each vector on host
		states = (int*)malloc(nodes*registers*bytes);
		messages = (int*)malloc(nodes*nodes*messtypes*bytes);

		// Ending states and messages memory
		ending_states = (int*)malloc(registers*bytes);
		ending_messages = (int*)malloc(messtypes*bytes);

		ending(ending_states,ending_messages,registers,messtypes);
 	}

	if (REST_T_CHECK(restrictions)) {
		if (verbose) printf("Ideal time complexity active, speeds will be ignored.\n");
		speed_lcm=1;
		speed_gcd=1;
	 	for (i=0 ; i < nodes ; i++) {
			*(nspeeds+i)=1;
		}

		for (i=0 ; i < nodes*nodes ; i++) {
			if (*(lspeeds+i)!=0) {
				*(lspeeds+i)=1;
			}
		}		
	} else {

		if (verbose) printf("Speed LCM %d:\n",speed_lcm);
		if (verbose) printf("Speed GCD %d:\n\n",speed_gcd);
	
		// Eventually rescaling the speed matrices
		if (speed_gcd!=1) {
		 	for (i=0 ; i < nodes ; i++) {
				*(nspeeds+i)=*(nspeeds+i)/speed_gcd;
			}
	
			for (i=0 ; i < nodes*nodes ; i++) {
				if (*(lspeeds+i)!=0) {
					*(lspeeds+i)=*(lspeeds+i)/speed_gcd;
				}
			}
	
			speed_lcm=speed_lcm/speed_gcd;
			speed_gcd=1;
	
			printf("Normalized Speed LCM %d:\n",speed_lcm);
			printf("Normalized Speed GCD %d:\n\n",speed_gcd);
		}
	}

	// Compute the increments and eventually is verbose print the speed matrix
	if (verbose) printf("Speed node matrix\n");
	for (i=0 ; i < nodes ; i++) {
		if (i==0) {
			if (verbose) printf("%d",*(nspeeds+i));
		} else {
			if (verbose) printf(" - %d",*(nspeeds+i));
		}
		*(ex_incr+i)=speed_lcm/(*(nspeeds+i));
	}
	if (verbose) printf("\n\n");

	// Compute the increments and eventually is verbose print the speed matrix
	if (verbose) printf("Speed links matrix\n");
	for (i=0,j=0 ; i < nodes*nodes ; i++) {
		if (*(lspeeds+i)!=0) {
			if (j==0) {
				if (verbose) printf("%d",*(lspeeds+i));
			} else {
				if (verbose) printf(" - %d",*(lspeeds+i));
			}
			*(link_incr+i)=speed_lcm/(*(lspeeds+i));
			j++;
		}
	}
	if (verbose) printf("\n\n");

	if (verbose) printf("Checking restrictions:\n");

	if (verbose) {
		if (REST_T_CHECK(restrictions)) {
			printf(" - Ideal time complexity\n");
		}
	}

	if (verbose) printf("\n");

	long int messcompl=0;


	// Device buffers
	cl_mem d_ex_next;
	cl_mem d_ex_incr;
	cl_mem d_ex_stat;
	cl_mem d_links;
	cl_mem d_states;
	cl_mem d_messages_in;
	cl_mem d_messages_out;
	cl_mem tempm;
 
	cl_platform_id cpPlatform;       // OpenCL platform
	cl_device_id device_id;           // device ID
	cl_context context;               // context
	cl_command_queue queue;           // command queue
	cl_program program;               // program
	cl_kernel kernel;                 // kernel


	size_t globalSize, localSize;
	cl_int err;
 
	// Number of work items in each local work group
	localSize = 1;
 
	// Number of total work items - localSize must be devisor
	globalSize = ceil(nodes/(int)localSize)*localSize;
 


	cl_platform_id * platform_ids;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;

	platform_ids=(cl_platform_id *) malloc(MAX_PLATFORMS*sizeof(cl_platform_id));
	err = clGetPlatformIDs(MAX_PLATFORMS, platform_ids, &ret_num_platforms);

	cl_device_id * device_ids;
	int dev_offset=0;
	device_ids=(cl_device_id *) malloc(MAX_DEVICES*sizeof(cl_device_id));

	if (verbose || show_platforms) printf("Found %d platforms\n", ret_num_platforms);

	if (verbose || show_platforms) {
		char* pname;
		pname=malloc(1024*sizeof(char));
		for (i=0 ; i < ret_num_platforms; i++) {
			clGetPlatformInfo(*(platform_ids+i),CL_PLATFORM_NAME,1024,pname,NULL);
			printf("\t%s\n",pname);
		}
		free(pname);
	}
	
	if (verbose || show_devices) printf("Devices\n");

	char* pname;
	pname=malloc(1024*sizeof(char));
	for (i=0 ; i < ret_num_platforms; i++) {
		err = clGetDeviceIDs(*(platform_ids+i), CL_DEVICE_TYPE_CPU, MAX_DEVICES-dev_offset, device_ids+dev_offset, &ret_num_devices);
		if (err == CL_SUCCESS) {
			for (j=dev_offset;j<ret_num_devices+dev_offset; j++) {
				clGetDeviceInfo(*(device_ids+j),CL_DEVICE_NAME,1024,pname,NULL);
				if (verbose || show_devices) printf("\t%d - %s",j,pname);
				if (choosen_device == j) {
					device_exists=1;
					device_id=*(device_ids+j);
					if (verbose || show_devices) printf(" - ###\n");
				} else {
					if (verbose || show_devices) printf("\n");
				}
			}
		}
		dev_offset=dev_offset+ret_num_devices;
		err = clGetDeviceIDs(*(platform_ids+i), CL_DEVICE_TYPE_GPU, MAX_DEVICES-dev_offset, device_ids+dev_offset, &ret_num_devices);
		if (err == CL_SUCCESS) {
			for (j=dev_offset;j<ret_num_devices+dev_offset; j++) {
				clGetDeviceInfo(*(device_ids+j),CL_DEVICE_NAME,1024,pname,NULL);
				if (verbose || show_devices) printf("\t%d - %s",j,pname);
				if (choosen_device == j) {
					device_exists=1;
					device_id=*(device_ids+j);
					if (verbose || show_devices) printf(" - ###\n");
				} else {
					if (verbose || show_devices) printf("\n");
				}
			}
			dev_offset=dev_offset+ret_num_devices;
		}
	}
	free(pname);

	if (!device_exists) {
		fprintf(stderr,"Failed to find a device!\n");
		return EXIT_FAILURE;
	}

	// Bind to platform
//	err = clGetPlatformIDs(2, cpPlatforms, NULL);
 
	// Get ID for the device
//	err = clGetDeviceIDs(*(cpPlatforms), CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
//	if (err != CL_SUCCESS)
//	{
//		fprintf(stderr, "Failed to create a device group!\n");
//		return EXIT_FAILURE;
//	}
 
	// Create a context 
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (!context)
	{
		fprintf(stderr, "Failed to create a compute context!\n");
		return EXIT_FAILURE;
	} 

	// Create a command queue
	queue = clCreateCommandQueue(context, device_id, 0, &err);
	if (!queue)
	{
		fprintf(stderr, "Failed to create a command queue!\n");
		return EXIT_FAILURE;
	}

 
	// Create the compute program from the source buffer
	program = clCreateProgramWithSource(context, 1, (const char **) & kernelSource, NULL, &err);
 
	// Build the program executable
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
    		fprintf(stderr, "Failed to build program executable!\n");
		return EXIT_FAILURE;
	}
 
	// Create the compute kernel in the program we wish to run
	kernel = clCreateKernel(program, entity, &err);
	if (!kernel || err != CL_SUCCESS)
	{
		fprintf(stderr, "Failed to create compute kernel!\n");
		return EXIT_FAILURE;
	}
 
	// Create the input and output arrays in device memory for our calculation
	d_ex_incr = clCreateBuffer(context, CL_MEM_READ_ONLY, nodes*bytes, NULL, NULL);
	d_ex_next = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*bytes, NULL, NULL);
	d_ex_stat = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*bytes, NULL, NULL);
	d_links = clCreateBuffer(context, CL_MEM_READ_ONLY, nodes*nodes*bytes, NULL, NULL);
	d_states = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*registers*bytes, NULL, NULL);
	d_messages_in = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*nodes*messtypes*bytes, NULL, NULL);
	d_messages_out = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*nodes*messtypes*bytes, NULL, NULL);

	if (!d_messages_in || !d_messages_out || !d_states || !d_links)
	{
		fprintf(stderr, "Failed to allocate device memory!\n");
		return EXIT_FAILURE;
	}

	// Write our data set into the input array in device memory
	err = clEnqueueWriteBuffer(queue, d_links, CL_TRUE, 0, nodes*nodes*bytes, links, 0, NULL, NULL);

	// Set the default according to the operation mode (the default is the temporal step -1 plus superposition of step 0) 
	if (opmode==1) {
		defaults(states,messages,nodes,-1);
		defaults(states,messages,nodes,0);
	} else {
		lua_defaults(L,ithnode,states,messages,nodes,-1,registers,messtypes,message_defaults);
		lua_defaults(L,ithnode,states,messages,nodes,0,registers,messtypes,message_defaults);
	}

	err |= clEnqueueWriteBuffer(queue, d_ex_stat, CL_TRUE, 0, nodes*bytes, ex_stat, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, d_ex_incr, CL_TRUE, 0, nodes*bytes, ex_incr, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, d_ex_next, CL_TRUE, 0, nodes*bytes, ex_next, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, d_states, CL_TRUE, 0, nodes*registers*bytes, states, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, d_messages_in, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);

	// Show matrices
	if (verbose) {
		printf("States matrix:\n");
		for (i=0 ; i < nodes ; i++) {
			printf("   Node %s registers: ",(*(ithnode+i))->name);
			for (j=0 ; j < registers ; j++) {
				tempstr=id_to_name(L,opmode,0,j,*(states+i*registers+j));
				if (tempstr != NULL) {
					printf("%s ",tempstr);
					free(tempstr);
				} else {
					printf("%d ",*(states+i*registers+j));
				}
			}
			printf("\n");
		}
		printf("\n");
		printf("Message matrix:\n");
		for (i=0 ; i < nodes ; i++) {
			for (j=0 ; j < nodes ; j++) {
				printf("   Node %s -> %s messages: ",(*(ithnode+i))->name,(*(ithnode+j))->name);
				for (k=0 ; k < messtypes ; k++) {
					tempstr=id_to_name(L,opmode,1,k,*(messages+(i*nodes+j)*messtypes+k));
					if (tempstr != NULL) {
						printf("%s ",tempstr);
						free(tempstr);
					} else {
						printf("%d ",*(messages+(i*nodes+j)*messtypes+k));
					}
				}
				printf("\n");
			}
			printf("\n");
		}
		printf("\n");
	}

	// The out_messages are set to default
	if (opmode==1) {
		defaults(states,messages,nodes,-1);
	} else {
		lua_defaults(L,ithnode,states,messages,nodes,-1,registers,messtypes,message_defaults);
	}

	err |= clEnqueueWriteBuffer(queue, d_messages_out, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);

	if (err != CL_SUCCESS)
	{
		fprintf(stderr, "Failed to write to source array!\n");
		return EXIT_FAILURE;
	}

	char * filen;
	filen = malloc(50*sizeof(char));


	if (verbose) printf("Starting simulation:\n");


	// Main simulation cycle
	for (timetick=1;timetick<sim_time;timetick++) {

		if (verbose) printf(" - Time %d:\n",timetick);
 
		// Set the arguments to our compute kernel
		err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_ex_stat);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_ex_incr);
		err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_ex_next);
		err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_links);
		err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_states);
		err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &d_messages_in);
		err |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &d_messages_out);
		err |= clSetKernelArg(kernel, 7, sizeof(unsigned int), &messtypes);
 		err |= clSetKernelArg(kernel, 8, sizeof(unsigned int), &registers);
		err |= clSetKernelArg(kernel, 9, sizeof(unsigned int), &nodes);
		err |= clSetKernelArg(kernel, 10, sizeof(unsigned int), &timetick);
		if (err != CL_SUCCESS)
		{
			fprintf(stderr, "Failed to set kernel arguments! %d\n", err);
			return EXIT_FAILURE;
		}
	 
		// Execute the kernel over the entire range of the data set 
		err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);
		if (err != CL_SUCCESS)
		{
			fprintf(stderr, "Failed to execute kernel!\n");
			return EXIT_FAILURE;
		}
	 
		// Wait for the command queue to get serviced before reading back results
		clFinish(queue);
	
		// Read the results from the device
		err=clEnqueueReadBuffer(queue, d_ex_next, CL_TRUE, 0, nodes*bytes, ex_next, 0, NULL, NULL );
		err=clEnqueueReadBuffer(queue, d_ex_stat, CL_TRUE, 0, nodes*bytes, ex_stat, 0, NULL, NULL );
		err|=clEnqueueReadBuffer(queue, d_states, CL_TRUE, 0, nodes*registers*bytes, states, 0, NULL, NULL );
		err|=clEnqueueReadBuffer(queue, d_messages_out, CL_TRUE, 0, nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL );

		if (pngout) {

			for (i=0 ; i < nodes ; i++) {
				style_node(L,opmode, styles, ithnode, i, states, nodes,registers);

				for (j=0 ; j < nodes ; j++) {
					if (*(links+i*nodes+j)!=0) {
						style_edge(L,opmode, styles, dsgraph,ithnode,i,j,messages,nodes,messtypes);
					}
				}
			}

			if (output_target==NULL) {
				output_target=strdup("dot");
			}

			if ((!strcmp(output_target,"dot"))||(!strcmp(output_target,"neato"))) {
				sprintf(filen,"outfile%04d.png",timetick);
				fp=fopen(filen,"w");
				gvLayout (gvc, dsgraph, output_target);
				gvRender (gvc, dsgraph, "png", fp);
				gvFreeLayout(gvc, dsgraph);
				fclose(fp);
			} else if (!strcmp(output_target,"gtk")) {
				gvLayout (gvc, dsgraph, output_target);
				gvRender (gvc, dsgraph, "gtk", fp);
				gvFreeLayout(gvc, dsgraph);
			}

		}

		if (ted) {
			int startdrawx;
			tedimx=1024;
			tedimy=768;
			tedim = gdImageCreateTrueColor(tedimx,tedimy);
			default_layout(&tedim,tedimx,tedimy);
			startdrawx=nodes_layout(&tedim,tedimx,tedimy, ithnode, nodes);
			step_layout(&tedim, tedimx, tedimy, messages, message_defaults, nodes, messtypes,timetick,&teddatas,startdrawx);
			sprintf(filen,"tedfile%04d.png",timetick);
			tedout=fopen(filen,"wb");
			gdImagePng(tedim,tedout);
			fclose(tedout);
			gdImageDestroy(tedim);
 		}

		if (err != CL_SUCCESS)
		{
			fprintf(stderr, "Failed to read output array! %d\n", err);
			return EXIT_FAILURE;
		}

		for (i=0 ; i < nodes ; i++) {
			if (verbose) {
				printf("   Node %s registers: ",(*(ithnode+i))->name);
				for (j=0 ; j < registers ; j++) {
					tempstr=id_to_name(L,opmode,0,j,*(states+i*registers+j));
					if (tempstr != NULL) {
						printf("%s ",tempstr);
						free(tempstr);
					} else {
						printf("%d ",*(states+i*registers+j));
					}
				}
				printf("\n");
				printf("   Node %s flags: Executed %d\n",(*(ithnode+i))->name,IS_RUN(*(ex_stat+i)));
			}
			if (verbose) {
				printf("   Node %s messages: ",(*(ithnode+i))->name);
			}

			for (j=0 ; j < nodes ; j++) {
				int messchck=0;
				for (k=0 ; k < messtypes ; k++) {
					if ( *(messages+(i*nodes+j)*messtypes+k)!=*(message_defaults+k)) {
						tempstr=id_to_name(L,opmode,1,k,*(messages+(i*nodes+j)*messtypes+k));
						if (tempstr != NULL) {
							if (verbose) printf("( %s -> %s ) ",tempstr,(*(ithnode+j))->name);
							free(tempstr);
						}
						messcompl++;
						messchck=1;
					}
				}

				if (messchck==1) {
					mes_queue_push(queues+(i*nodes+j), messtypes, messages+(i*nodes+j)*messtypes,timetick+*(link_incr+(i*nodes+j)));
//					printf("Queue lenght: %d ",mes_queue_count(queues+(i*nodes+j), messtypes));
//					mes_queue_trav(queues+(i*nodes+j), messtypes);
				}
			}

			if (verbose) printf("\n");
		}
		if (verbose) printf("\n");

		if (bypass_ending==0) {	
			if (check_end(states,messages,nodes,ending_states,ending_messages,registers,messtypes) == 1) break;
		}

		// Resetting the messages array to defaults
		if (opmode==1) {
			defaults(states,messages,nodes,-1);
		} else {
			lua_defaults(L,ithnode,states,messages,nodes,-1,registers,messtypes,message_defaults);
		}

		err = clEnqueueWriteBuffer(queue, d_messages_out, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);

		for (i=0 ; i < nodes ; i++) {
			for (j=0 ; j < nodes ; j++) {
				if ((*(ex_next+j)<=timetick+1)&&(queues+(i*nodes+j)!=NULL)) {
					int * message;
//					mes_queue_trav(queues+(i*nodes+j), messtypes);
					message=mes_queue_timedpop(queues+(i*nodes+j),messtypes,timetick+1);
					if (message!=NULL) {
//						printf("PUNT out %d %d %p\n",i,j,message);
						for(l=0; l<messtypes; l++){
							*(messages+(i*nodes+j)*messtypes+l)=*(message+l);
//							printf("--- %d \n",*(message+l));
						}
						free(message);
					}
				}
			}
		}

		err = clEnqueueWriteBuffer(queue, d_messages_in, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);
		clFinish(queue);

//		tempm=d_messages_in;
//		d_messages_in=d_messages_out;
//		d_messages_out=tempm;

	}

	if (lua_check_report(L, "TIME_COMPLEXITY") == 1) printf("Time Complexity: %d\n",timetick);
	if (lua_check_report(L, "MESSAGE_COMPLEXITY") == 1) printf("Message Complexity: %ld\n",messcompl);


	// release OpenCL resources
	clReleaseMemObject(d_ex_next);
	clReleaseMemObject(d_ex_incr);
	clReleaseMemObject(d_ex_stat);
	clReleaseMemObject(d_links);
	clReleaseMemObject(d_states);
	clReleaseMemObject(d_messages_in);
	clReleaseMemObject(d_messages_out);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	
	//release host memory
	free(ex_next);
	free(ex_incr);
	free(ex_stat);
	free(nspeeds);
	free(lspeeds);
	free(link_incr);
	free(queues);
	free(links);
	free(states);
	free(messages);
	free(message_defaults);
	free(entity);

	// Release graph(viz) resources
	agclose(dsgraph);
	gvFreeContext(gvc);

//	if (verbose) printf("\nLua stack size %d\n\n",lua_gettop(L));

	// Release LUA interpreter
	lua_close(L);

	return 0;
}
