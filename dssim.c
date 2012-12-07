#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <CL/opencl.h>
#include <graphviz/gvc.h>
#include <graphviz/graph.h>

#define MAX_SOURCE_SIZE (0x100000)
 
// Enable double precision values
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable
 
// OpenCL kernels: the kernel is the protocol
//const char *kernelSource =                                      "\n" \
//"__kernel void nodengine(  __global float *links,                \n" \
//"                          __global float *states,               \n" \
//"                          __global float *messages,             \n" \
//"                          const unsigned int messtypes,         \n" \
//"                          const unsigned int registers,         \n" \
//"                          const unsigned int nodes)             \n" \
//"{                                                               \n" \
//"    //Get our global thread ID                                  \n" \
//"    int id = get_global_id(0);                                  \n" \
//"                                                                \n" \
//"    //Make sure we do not go out of bounds                      \n" \
//"    if (id < nodes)                                             \n" \
//"        messages[id] = links[id] + states[id];                  \n" \
//"}                                                               \n" \
//

//const char *kernelSource =                                      "\n" \
//"#define STATE_INIT 1.0                                          \n" \
//"#define STATE_IDLE 0.0                                          \n" \
//"#define STATE_DONE -1.0                                          \n" \
//"                                                                \n" \
//"#define MESSAGE_BRD 1.0                                         \n" \
//"                                                                \n" \
//"__kernel void broadcast(  __global int *links,                \n" \
//"                          __global int *states,               \n" \
//"                          __global int *messages_in,          \n" \
//"                          __global int *messages_out,         \n" \
//"                          const unsigned int messtypes,         \n" \
//"                          const unsigned int registers,         \n" \
//"                          const unsigned int nodes)             \n" \
//"{                                                               \n" \
//"    //Get our global thread ID                                  \n" \
//"    int nid = get_global_id(0);                                 \n" \
//"                                                                \n" \
//"    //Make sure we do not go out of bounds                      \n" \
//"    if (nid < nodes) {                                          \n" \
//"                                                                \n" \
//"    //Start the entity work                                     \n" \
//"    int mystate = states[nid];                                  \n" \
//"                                                                \n" \
//"//    if (mystate == STATE_INIT) {                                                            \n" \
//"                                                                \n" \
//"//        int i,j,nli=0;                                          \n" \
//"//        for (i=0;i<nodes;i++) {                                 \n" \
//"//            if (links[nid*nodes+i] > 0.0) {                     \n" \
//"//                nli=nli+1;                                      \n" \
//"//            }                                                   \n" \
//"//        }                                                       \n" \
//"                                                                \n" \
//"        states[nid*registers] = STATE_DONE;                     \n" \
//"    }                                                           \n" \
//"}                                                               \n" \
//                                                                "\n" ;

void version()
{
	printf("DSSim - Distributed System OpenCL Simulator\nCopyright 2012 - Mirko Mariotti - http://www.mirkomariotti.it\n");
	fflush(stdout);
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

int main( int argc, char* argv[] )
{
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
	graph_t * dsgraph;

	Agnode_t * inode;
	Agedge_t * iedge;

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

	// Entity to load
	char * entity;
	char * entityfile;

	///// Program start

	// Size, in bytes, of each vector
	size_t bytes = sizeof(int);

	// Start with the command line parsing
	while ((c = getopt (argc, argv, "vVk:p:g:i:")) != -1)
	switch (c) {
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
                case '?':
                        if ((optopt == 'k')||(optopt == 'p')||(optopt == 'g')||(optopt == 'i'))
                        {
                                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                                exit(1);
                        }
                        else
                        {
                                if (isprint (optopt))
                                {
                                        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                                        exit(1);
                                }
                                else
                                {
                                        fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
                                        exit(1);
                                }
                        }
                default:
                        return 1;
	}

	// Exits on wrong options
	for (index = optind; index < argc; index++) {
		fprintf (stderr,"Non-option argument %s\n", argv[index]);
		exit(1);
	}

	if (verbose) {
		version();
		printf("\n-----\n\n");
	}

	// Check the operation mode
	if ((protocol_file==NULL)&&(kernel_file==NULL)) {
		fprintf (stderr,"Either a protocol file or a custom opencl kernel is required.");
		exit(1);
	} else if ((protocol_file!=NULL)&&(kernel_file!=NULL)) {
		fprintf (stderr,"A protocol file or a custom opencl kernel is required (not both).");
		exit(1);
	} else if ((protocol_file!=NULL)&&(kernel_file==NULL)) {
///// TODO
	} else if ((protocol_file==NULL)&&(kernel_file!=NULL)) {
		entityfile=kernel_file;
		entity= (char *) malloc((int) strlen((entityfile) - 2)*sizeof(char));
		strncpy(entity,entityfile,(int) strlen(entityfile) - 3);
	}

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
		fprintf (stderr,"No graph given, you need to use the -g option.\n");
		exit(1);
	}

	//////// The configuration part start here


	// Message types
	unsigned int messtypes = 1;

	// Node registers
	unsigned int registers = 1;
 
 	// The states matrix is a nodes x registers which stores for each entity the value of the given register. Two things:
 	// 1 - The register value if up to the opencl kernel, it is relevant to the host only for analysis purposes.
 	// 2 - The first register is the state register and has to exist.
	int *states;

 	// The messages matrix is a nodes x nodes x messtypes which stores for each x to y comunication if the message of type z has arrived
	int *messages;


	///////// The configuration part stops here

	int doneck;

	long int messcompl=0;

 
	// Allocate memory for each vector on host
	states = (int*)malloc(nodes*registers*bytes);
	messages = (int*)malloc(nodes*nodes*messtypes*bytes);

	for (i=0;i<nodes*registers;i++) *(states+i)=0;
	for (i=0;i<nodes*nodes*messtypes;i++) *(messages+i)=0;

	// Load the kernel source code into the array kernelSource
	char *kernelSource;
	size_t source_size;

	fp = fopen(entityfile, "r");
	if (!fp)
	{
		fprintf(stderr, "Failed load the kernel!\n");
		return EXIT_FAILURE;
	}
	kernelSource = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread( kernelSource, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);
 
	// Device buffers
	cl_mem d_links;
	cl_mem d_states;
	cl_mem d_messages_in;
	cl_mem d_messages_out;
	cl_mem tempm;
 
	cl_platform_id cpPlatform;        // OpenCL platform
	cl_device_id device_id;           // device ID
	cl_context context;               // context
	cl_command_queue queue;           // command queue
	cl_program program;               // program
	cl_kernel kernel;                 // kernel


	size_t globalSize, localSize;
	cl_int err;
 
	// Number of work items in each local work group
	localSize = 5;
 
	// Number of total work items - localSize must be devisor
	globalSize = ceil(nodes/(int)localSize)*localSize;
 
	// Bind to platform
	err = clGetPlatformIDs(1, &cpPlatform, NULL);
 
	// Get ID for the device
	err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	if (err != CL_SUCCESS)
	{
		fprintf(stderr, "Failed to create a device group!\n");
		return EXIT_FAILURE;
	}
 
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

*(states)=1;
//*(states+4)=1;
//	for (i=0;i<nodes;i++) {
//		*(states+i)=1;
//	}

	err |= clEnqueueWriteBuffer(queue, d_states, CL_TRUE, 0, nodes*registers*bytes, states, 0, NULL, NULL);
*(messages)=1;
//*(messages+24)=1;

//	for (i=0;i<nodes;i++) {
//		for (j=0;j<nodes;j++) {
//			if (i==j) *(messages+i*nodes+j)=1;
//		}
//	}

	err |= clEnqueueWriteBuffer(queue, d_messages_in, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);
	for (i=0;i<nodes*nodes;i++) {
		*(messages+i)=0;
	}
	err |= clEnqueueWriteBuffer(queue, d_messages_out, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);

	if (err != CL_SUCCESS)
	{
		fprintf(stderr, "Failed to write to source array!\n");
		return EXIT_FAILURE;
	}

	if (verbose) printf("Staring simulation:\n",l);

	// Main simulation cycle
	for (l=0;l<1000;l++) {

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
	
		if (err != CL_SUCCESS)
		{
			fprintf(stderr, "Failed to read output array! %d\n", err);
			return EXIT_FAILURE;
		}
	
		doneck=1;
		for (i=0 ; i < nodes ; i++) {
			if (verbose) {
				printf("   Node %s registers: ",(*(ithnode+i))->name);
				for (j=0 ; j < registers ; j++) {
					printf("%d ",*(states+i*registers+j));
				}
				printf("\n");
	
				printf("   Node %s messages: ",(*(ithnode+i))->name);
				for (j=0 ; j < nodes ; j++) {
					for (k=0 ; k < messtypes ; k++) {
						printf("( %d -> %d = %d ) ",i,j,*(messages+(i*nodes+j)*messtypes+k));
						if ( *(messages+(i*nodes+j)*messtypes+k)==1) messcompl++;
					}
				}
				printf("\n");
			}
			if (*(states+i*registers+0)!=-1) {
				doneck=0;
			}
		}
		printf("\n");
	
		if (doneck==1) break;
	
		tempm=d_messages_in;
		d_messages_in=d_messages_out;
		d_messages_out=tempm;
	}

	printf("Time Complexity: %d\n",l+1);
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

	return 0;
}
