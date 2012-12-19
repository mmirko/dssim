#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <CL/opencl.h>
#include <graphviz/gvc.h>
#include <graphviz/graph.h>


void version()
{
	printf("DSSim - Distributed System OpenCL Simulator\nCopyright 2012 - Mirko Mariotti - http://www.mirkomariotti.it\n");
	fflush(stdout);
}

void usage()
{
	printf("DSSim - Distributed System OpenCL Simulator\nCopyright 2012 - Mirko Mariotti - http://www.mirkomariotti.ii\nUsage:\n\n");
	printf("\tdssim -g graph_dot_file -p protocol_file -i init_file [-s OpenCL_custom_kernel] [-t time] [-v] [-o]\n");
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
	printf("\t\t-t file  - Set the simulation time (default 10000)\n");
	printf("\t\t-o       - Generate a PNG file for each simulation step\n");
	fflush(stdout);

}

Agraph_t * hypercube(GVC_t* gvc, int dim)
{
	Agraph_t * result=NULL;
	if (dim==1) {
		Agnode_t *n1, *n2;
		Agedge_t *e, *f;

		result=agopen("dsgraph", AGDIGRAPH);
		n1 = agnode(result, "0");
		n2 = agnode(result, "1");
		e = agedge(result, n1, n2);
		f = agedge(result, n2, n1);
		agsafeset(e, "index", "1", "");
		agsafeset(f, "index", "1", "");
	} else {
		Agraph_t * g1;
		Agraph_t * g2;

		Agnode_t *n1, *n2;
		Agedge_t *e, *f;

		Agnode_t *inode;
		Agedge_t *iedge;

		char tempstr1[dim+1];
		char tempstr2[dim+1];
		char * temps;

		g1=hypercube(gvc,dim-1);
		g2=hypercube(gvc,dim-1);

		result=agopen("dsgraph", AGDIGRAPH);

		for (inode=agfstnode(g1);inode!=NULL;inode=agnxtnode(g1,inode)) {
			tempstr1[0]='0';
			strncpy(tempstr1+1,inode->name,dim-1);
			tempstr1[dim]=0;
			n1 = agnode(result, tempstr1);

		}
		for (inode=agfstnode(g2);inode!=NULL;inode=agnxtnode(g2,inode)) {
			tempstr2[0]='1';
			strncpy(tempstr2+1,inode->name,dim-1);
			tempstr2[dim]=0;
			n1 = agnode(result, tempstr2);
		}

		for (inode=agfstnode(g1);inode!=NULL;inode=agnxtnode(g1,inode)) {
			for (iedge=agfstout(g1,inode);iedge!=NULL;iedge=agnxtout(g1,iedge)) {
				temps=agget(iedge,"index");	
				tempstr1[0]='0';
				strncpy(tempstr1+1,iedge->head->name,dim-1);
				tempstr1[dim]=0;
				tempstr2[0]='0';
				strncpy(tempstr2+1,iedge->tail->name,dim-1);
				tempstr2[dim]=0;
				n1=agnode(result, tempstr1);
				n2=agnode(result, tempstr2);
				e=agedge(result,n1,n2);
				agsafeset(e, "index", temps, "");
			}
		}

		for (inode=agfstnode(g2);inode!=NULL;inode=agnxtnode(g2,inode)) {
			for (iedge=agfstout(g2,inode);iedge!=NULL;iedge=agnxtout(g2,iedge)) {
				temps=agget(iedge,"index");	
				tempstr1[0]='1';
				strncpy(tempstr1+1,iedge->head->name,dim-1);
				tempstr1[dim]=0;
				tempstr2[0]='1';
				strncpy(tempstr2+1,iedge->tail->name,dim-1);
				tempstr2[dim]=0;
				n1=agnode(result, tempstr1);
				n2=agnode(result, tempstr2);
				e=agedge(result,n1,n2);
				agsafeset(e, "index", temps, "");
			}
		}

		for (inode=agfstnode(g1);inode!=NULL;inode=agnxtnode(g1,inode)) {
			temps=(char *) malloc(50*sizeof(char));
			sprintf(temps,"%d",dim);
			tempstr1[0]='0';
			strncpy(tempstr1+1,inode->name,dim-1);
			tempstr1[dim]=0;
			tempstr2[0]='1';
			strncpy(tempstr2+1,inode->name,dim-1);
			tempstr2[dim]=0;
			n1=agnode(result, tempstr1);
			n2=agnode(result, tempstr2);
			e=agedge(result,n1,n2);
			agsafeset(e, "index", temps, "");
			f=agedge(result,n2,n1);
			agsafeset(f, "index", temps, "");
			free(temps);
		}
	}
	return result;
}



int main( int argc, char* argv[] )
{
	// Counters
	int c,index;
	int i,j,k,l;

	// Verbose flag
	char verbose=0;

	// Graph(viz) managment
	GVC_t* gvc;
	Agraph_t * dsgraph;

	Agnode_t * inode;
	Agedge_t * iedge;

	// Start with the command line parsing
	while ((c = getopt (argc, argv, "hvV")) != -1)
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

	// Open graphviz context
	gvc = gvContext();

	dsgraph=hypercube(gvc,3);

	gvLayout (gvc, dsgraph,"dot");
	gvRender (gvc, dsgraph,"dot",stdout);
	gvFreeLayout(gvc,dsgraph);

	agclose (dsgraph);
	gvFreeContext(gvc);
}
