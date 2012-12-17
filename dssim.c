#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <CL/opencl.h>
#include <graphviz/gvc.h>
#include <graphviz/graph.h>

#define MAX_SOURCE_SIZE (0x100000)
 
#include "transformer.c"

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


void lua_defaults(lua_State *L,Agnode_t **  ithnode,  int * states, int * messages, int nodes, int step, int registers, int messtypes)
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

	// lev2 == -1 means to resolve the name of the level1
	if (lev2 == -1 ) {
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
	printf("DSSim - Distributed System OpenCL Simulator\nCopyright 2012 - Mirko Mariotti - http://www.mirkomariotti.it\n");
	fflush(stdout);
}

void usage()
{
	printf("DSSim - Distributed System OpenCL Simulator\nCopyright 2012 - Mirko Mariotti - http://www.mirkomariotti.ii\nUsage:\n\n");
	printf("\tdssim -g graph_dot_file -p protocol_file -i init_file [-v][-o]\n");
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
	printf("\t\t-o       - Generate a PNG file for each simulation step\n");
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

int main( int argc, char* argv[] )
{
	// Temp string
	char * tempstr;

	// Counters
	int c,index;
	int i,j,k,l;

	// The protocol filename
	char * protocol_file = NULL;

	// The graph filename
	char * graph_file = NULL;

	// The custom kernel filename
	char * kernel_file = NULL;

	// The initial condition filename
	char * initial_file = NULL;

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

 	// The links matrix is a nodes x nodes which stores 0 if there in not a link x->y, !=0 otherwise (in the future it may contain same sort of link weight)
	int * links;

	// Node registers
	unsigned int registers = REGISTERS;

	// Message types
	unsigned int messtypes = MESSTYPES;

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

	///// Program start

	// Size, in bytes, of each vector
	size_t bytes = sizeof(int);

	// Start with the command line parsing
	while ((c = getopt (argc, argv, "hvVk:p:g:i:o")) != -1)
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
		case 'o':
			pngout=1;
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

		// Allocate memory for the link matrix and initialize it
		links = (int*)malloc(nodes*nodes*bytes);
		for (i=0;i<nodes*nodes;i++) *(links+i)=0;

		// Populate the id->node resolution
		for (inode=agfstnode(dsgraph),i=0;inode!=NULL;inode=agnxtnode(dsgraph,inode),i++) {
			if (verbose) printf(" - Importing node - %s (Host %p)\n",inode->name,inode);
			*(ithnode+i)=inode;
		}

		if (verbose) printf("%d nodes imported\n\n",nodes);

		if (verbose) printf("Importing graph edges:\n");

		for (inode=agfstnode(dsgraph);inode!=NULL;inode=agnxtnode(dsgraph,inode)) {
			if (verbose) printf(" - Importing node %s edges:\n",inode->name);
			for (iedge=agfstout(dsgraph,inode);iedge!=NULL;iedge=agnxtout(dsgraph,iedge)) {
				if (verbose) printf("   - Importing edge - %s (Host %p) -> %s (Host %p)\n",iedge->tail->name,iedge->tail,iedge->head->name,iedge->head);
				// Find the two indexes
				j=search_node_id(ithnode,iedge->tail,nodes);
				k=search_node_id(ithnode,iedge->head,nodes);
				*(links+j*nodes+k)=1;
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

		// Allocate memory for each vector on host
		states = (int*)malloc(nodes*registers*bytes);
		messages = (int*)malloc(nodes*nodes*messtypes*bytes);

		// Ending states and messages memory
		ending_states = (int*)malloc(registers*bytes);
		ending_messages = (int*)malloc(messtypes*bytes);

		// Populate the ending conditions
		lua_ending(L,ending_states,ending_messages,registers,messtypes);	

		// Populate the styles
		styles=lua_styles(L);
		if (verbose) {
			printf("Loading graphical styles:\n");
			styles_print(styles);
		}

		// Find out the number of messages types
		lua_getglobal(L, "transformer");
		lua_pushstring(L,protocol_file);
		lua_call(L,1,1);

		kernelSource=strdup(lua_tostring(L, -1));
		lua_pop(L,1);
		entity=protocol_file;


		if (verbose) {
			printf("Produced OpenCL kernel:\n\n");
			printf("%s",kernelSource);
			printf("\n----\n");
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


	int doneck;

	long int messcompl=0;


	// Device buffers
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

	platform_ids=(cl_platform_id *) malloc(2*sizeof(cl_platform_id));
	err = clGetPlatformIDs(2, platform_ids, &ret_num_platforms);

	printf("Found %d platforms\n", ret_num_platforms);

	for (i=0 ; i < ret_num_platforms; i++)
	{
		err = clGetDeviceIDs(*(platform_ids+i), CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
		if (ret_num_devices==1) break;
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
		lua_defaults(L,ithnode,states,messages,nodes,-1,registers,messtypes);
		lua_defaults(L,ithnode,states,messages,nodes,0,registers,messtypes);
	}

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
		lua_defaults(L,ithnode,states,messages,nodes,-1,registers,messtypes);
	}

	err |= clEnqueueWriteBuffer(queue, d_messages_out, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);

	if (err != CL_SUCCESS)
	{
		fprintf(stderr, "Failed to write to source array!\n");
		return EXIT_FAILURE;
	}

	if (verbose) printf("Starting simulation:\n",l);

	char * filen;
	filen = malloc(50*sizeof(char));

	// Main simulation cycle
	for (l=1;l<60;l++) {

		if (verbose) printf(" - Time %d:\n",l);
 
		// Set the arguments to our compute kernel
		err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_links);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_states);
		err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_messages_in);
		err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_messages_out);
		err |= clSetKernelArg(kernel, 4, sizeof(unsigned int), &messtypes);
 		err |= clSetKernelArg(kernel, 5, sizeof(unsigned int), &registers);
		err |= clSetKernelArg(kernel, 6, sizeof(unsigned int), &nodes);
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
		err=clEnqueueReadBuffer(queue, d_states, CL_TRUE, 0, nodes*registers*bytes, states, 0, NULL, NULL );
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

			sprintf(filen,"outfile%04d.png",l);
			fp=fopen(filen,"w");
			gvLayout (gvc, dsgraph, "dot");
			gvRender (gvc, dsgraph, "png", fp);
			gvFreeLayout(gvc, dsgraph);
			fclose(fp);

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
			}
			if (verbose) {
				printf("   Node %s messages: ",(*(ithnode+i))->name);
			}

			for (j=0 ; j < nodes ; j++) {
				for (k=0 ; k < messtypes ; k++) {
					if ( *(messages+(i*nodes+j)*messtypes+k)!=0) {
						tempstr=id_to_name(L,opmode,1,k,*(messages+(i*nodes+j)*messtypes+k));
						if (tempstr != NULL) {
							if (verbose) printf("( %s -> %s ) ",tempstr,(*(ithnode+j))->name);
							free(tempstr);
						}
						messcompl++;
					}
				}
			}

			if (verbose) printf("\n");
			
			if (*(states+i*registers+0)!=-1) {
				doneck=0;
			}
		}
		if (verbose) printf("\n");
	
		if (check_end(states,messages,nodes,ending_states,ending_messages,registers,messtypes) == 1) break;
	
		tempm=d_messages_in;
		d_messages_in=d_messages_out;
		d_messages_out=tempm;
	}



	printf("Time Complexity: %d\n",l);
	printf("Message Complexity: %ld\n",messcompl);
 
	// release OpenCL resources
	clReleaseMemObject(d_links);
	clReleaseMemObject(d_states);
	clReleaseMemObject(d_messages_in);
	clReleaseMemObject(d_messages_out);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	
	//release host memory
	free(links);
	free(states);
	free(messages);
	free(entity);

	// Release graph(viz) resources
	agclose(dsgraph);
	gvFreeContext(gvc);

//	if (verbose) printf("\nLua stack size %d\n\n",lua_gettop(L));

	// Release LUA interpreter
	lua_close(L);

	return 0;
}
