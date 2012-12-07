#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CL/opencl.h>
#include <graphviz/gvc.h>

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
 
int main( int argc, char* argv[] )
{
	//////// The configuration part start here

	// Entity to load
	const char * entity="broadcast";
	const char * entityfile="broadcast.cl";

	// Node numbers
	unsigned int nodes = 5;

	// Message types
	unsigned int messtypes = 1;

	// Node registers
	unsigned int registers = 1;
 
 	// The links matrix is a nodes x nodes which stores 0 if there in not a link x->y, !=0 otherwise (in the future it may contain same sort of link weight)
//	int * links;

	int links[]={
			0, 1, 0, 0, 0,
			1, 0, 1, 0, 0,
			0, 1, 0, 1, 1,
			0, 0, 1, 0, 0,
			0, 0, 1, 0, 0
	};

 	// The states matrix is a nodes x registers which stores for each entity the value of the given register. Two things:
 	// 1 - The register value if up to the opencl kernel, it is relevant to the host only for analysis purposes.
 	// 2 - The first register is the state register and has to exist.
	int *states;

 	// The messages matrix is a nodes x nodes x messtypes which stores for each x to y comunication if the message of type z has arrived
	int *messages;


	///////// The configuration part stops here

	int i,j,k,l;
	int doneck;

	long int messcompl=0;

	// Size, in bytes, of each vector
	size_t bytes = sizeof(int);
 
	// Allocate memory for each vector on host
//	links = (int*)malloc(nodes*nodes*bytes);
	states = (int*)malloc(nodes*registers*bytes);
	messages = (int*)malloc(nodes*nodes*messtypes*bytes);

//	for (i=0;i<nodes*nodes;i++) *(links+i)=0;
	for (i=0;i<nodes*registers;i++) *(states+i)=0;
	for (i=0;i<nodes*nodes*messtypes;i++) *(messages+i)=0;

	// Load the kernel source code into the array kernelSource
	FILE *fp;
	char *kernelSource;
	size_t source_size;

	fp = fopen(entityfile, "r");
	if (!fp)
	{
		printf("Error: Failed load the kernel!\n");
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
		printf("Error: Failed to create a device group!\n");
		return EXIT_FAILURE;
	}
 
	// Create a context 
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (!context)
	{
		printf("Error: Failed to create a compute context!\n");
		return EXIT_FAILURE;
	} 

	// Create a command queue
	queue = clCreateCommandQueue(context, device_id, 0, &err);
	if (!queue)
	{
		printf("Error: Failed to create a command queue!\n");
		return EXIT_FAILURE;
	}

 
	// Create the compute program from the source buffer
	program = clCreateProgramWithSource(context, 1, (const char **) & kernelSource, NULL, &err);
 
	// Build the program executable
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
    		printf("Error: Failed to build program executable!\n");
		return EXIT_FAILURE;
	}
 
	// Create the compute kernel in the program we wish to run
	kernel = clCreateKernel(program, entity, &err);
	if (!kernel || err != CL_SUCCESS)
	{
		printf("Error: Failed to create compute kernel!\n");
		return EXIT_FAILURE;
	}
 
	// Create the input and output arrays in device memory for our calculation
	d_links = clCreateBuffer(context, CL_MEM_READ_ONLY, nodes*nodes*bytes, NULL, NULL);
	d_states = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*registers*bytes, NULL, NULL);
	d_messages_in = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*nodes*messtypes*bytes, NULL, NULL);
	d_messages_out = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes*nodes*messtypes*bytes, NULL, NULL);

	if (!d_messages_in || !d_messages_out || !d_states || !d_links)
	{
		printf("Error: Failed to allocate device memory!\n");
		return EXIT_FAILURE;
	}

	// Write our data set into the input array in device memory
	err = clEnqueueWriteBuffer(queue, d_links, CL_TRUE, 0, nodes*nodes*bytes, links, 0, NULL, NULL);

//*(states)=1;
//*(states+4)=1;
	for (i=0;i<nodes;i++) {
		*(states+i)=1;
	}

	err |= clEnqueueWriteBuffer(queue, d_states, CL_TRUE, 0, nodes*registers*bytes, states, 0, NULL, NULL);
//*(messages)=1;
//*(messages+24)=1;

	for (i=0;i<nodes;i++) {
		for (j=0;j<nodes;j++) {
			if (i==j) *(messages+i*nodes+j)=1;
		}
	}

	err |= clEnqueueWriteBuffer(queue, d_messages_in, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);
	for (i=0;i<nodes*nodes;i++) {
		*(messages+i)=0;
	}
	err |= clEnqueueWriteBuffer(queue, d_messages_out, CL_TRUE, 0,  nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL);

	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to write to source array!\n");
		return EXIT_FAILURE;
	}

for (l=0;l<6;l++) {
 
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
		printf("Error: Failed to set kernel arguments! %d\n", err);
		return EXIT_FAILURE;
	}
 
	// Execute the kernel over the entire range of the data set 
	err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to execute kernel!\n");
		return EXIT_FAILURE;
	}
 
	// Wait for the command queue to get serviced before reading back results
	clFinish(queue);

	// Read the results from the device
	err=clEnqueueReadBuffer(queue, d_states, CL_TRUE, 0, nodes*registers*bytes, states, 0, NULL, NULL );
	err|=clEnqueueReadBuffer(queue, d_messages_out, CL_TRUE, 0, nodes*nodes*messtypes*bytes, messages, 0, NULL, NULL );

	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to read output array! %d\n", err);
		return EXIT_FAILURE;
	}

doneck=1;
	for (i=0 ; i < nodes ; i++) {
		printf("Node %d registers: ",i);
		for (j=0 ; j < registers ; j++) {
			printf("%d ",*(states+i*registers+j));
		}
		printf("\n");

		printf("Node %d messages: ",i);
		for (j=0 ; j < nodes ; j++) {
			for (k=0 ; k < messtypes ; k++) {
				printf("( %d -> %d = %d ) ",i,j,*(messages+(i*nodes+j)*messtypes+k));
				if ( *(messages+(i*nodes+j)*messtypes+k)==1) messcompl++;
			}
		}
		printf("\n");

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
//	free(links);
	free(states);
	free(messages);
	
	return 0;
}
