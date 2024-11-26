#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <graphviz/gvc.h>
#include <graphviz/cgraph.h>


void version()
{
	printf("DSSimGen - Distributed System OpenCL Simulator\nCopyright 2024 - Mirko Mariotti - https://www.mirkomariotti.it\n");
	fflush(stdout);
}

void usage()
{
	printf("DSSimGen - Distributed System OpenCL Simulator\nCopyright 2024 - Mirko Mariotti - https://www.mirkomariotti.ii\nUsage:\n\n");
	printf("\tdssim_gen [-v] -t graph_type -p graph_parameters\n");
	printf("\tdssim_gen -h\n");
	printf("\tdssim_gen -V\n\n");
	printf("\tOptions:\n");
	printf("\t\t-h       - This help\n");
	printf("\t\t-v       - Be verbose\n");
	printf("\t\t-V       - Print program version and exit\n");

	fflush(stdout);

}

Agraph_t * hypercube(GVC_t* gvc, int dim)
{
	Agraph_t * result=NULL;
	if (dim==1) {
		Agnode_t *n1, *n2;
		Agedge_t *e, *f;

		result=agopen("dsgraph", Agdirected, NULL);
		n1 = agnode(result, "0",true);
		n2 = agnode(result, "1",true);
		e = agedge(result, n1, n2, NULL, true);
		f = agedge(result, n2, n1, NULL, true);
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

		result=agopen("dsgraph", Agdirected, NULL);

		for (inode=agfstnode(g1);inode!=NULL;inode=agnxtnode(g1,inode)) {
			tempstr1[0]='0';
			strncpy(tempstr1+1,agnameof(inode),dim-1);
			tempstr1[dim]=0;
			n1 = agnode(result, tempstr1,true);

		}
		for (inode=agfstnode(g2);inode!=NULL;inode=agnxtnode(g2,inode)) {
			tempstr2[0]='1';
			strncpy(tempstr2+1,agnameof(inode),dim-1);
			tempstr2[dim]=0;
			n1 = agnode(result, tempstr2,true);
		}

		for (inode=agfstnode(g1);inode!=NULL;inode=agnxtnode(g1,inode)) {
			for (iedge=agfstout(g1,inode);iedge!=NULL;iedge=agnxtout(g1,iedge)) {
				temps=agget(iedge,"index");	
				tempstr1[0]='0';
				strncpy(tempstr1+1,agnameof(aghead(iedge)),dim-1);
				tempstr1[dim]=0;
				tempstr2[0]='0';
				strncpy(tempstr2+1,agnameof(agtail(iedge)),dim-1);
				tempstr2[dim]=0;
				n1=agnode(result, tempstr1,true);
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2, NULL, true);
				agsafeset(e, "index", temps, "");
			}
		}

		for (inode=agfstnode(g2);inode!=NULL;inode=agnxtnode(g2,inode)) {
			for (iedge=agfstout(g2,inode);iedge!=NULL;iedge=agnxtout(g2,iedge)) {
				temps=agget(iedge,"index");	
				tempstr1[0]='1';
				strncpy(tempstr1+1,agnameof(aghead(iedge)),dim-1);
				tempstr1[dim]=0;
				tempstr2[0]='1';
				strncpy(tempstr2+1,agnameof(agtail(iedge)),dim-1);
				tempstr2[dim]=0;
				n1=agnode(result, tempstr1,true);
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2, NULL, true);
				agsafeset(e, "index", temps, "");
			}
		}

		for (inode=agfstnode(g1);inode!=NULL;inode=agnxtnode(g1,inode)) {
			temps=(char *) malloc(50*sizeof(char));
			sprintf(temps,"%d",dim);
			tempstr1[0]='0';
			strncpy(tempstr1+1,agnameof(inode),dim-1);
			tempstr1[dim]=0;
			tempstr2[0]='1';
			strncpy(tempstr2+1,agnameof(inode),dim-1);
			tempstr2[dim]=0;
			n1=agnode(result, tempstr1,true);
			n2=agnode(result, tempstr2,true);
			e=agedge(result,n1,n2, NULL, true);
			agsafeset(e, "index", temps, "");
			f=agedge(result,n2,n1, NULL, true);
			agsafeset(f, "index", temps, "");
			free(temps);
		}
	}
	return result;
}

Agraph_t * toroid (GVC_t* gvc, int n, int m)
{
	int i,j;

	Agraph_t * result=NULL;

	Agnode_t *n1, *n2;
	Agedge_t *e;

	char tempstr1[11];
	char tempstr2[11];

	result=agopen("dsgraph", Agdirected, NULL);

	for (i=0;i<n;i++) {
		for (j=0;j<m;j++) {
			sprintf(tempstr1,"X%04dY%04d",i,j);
			tempstr1[10]=0;
			n1=agnode(result, tempstr1,true);
		}
	}

	for (i=0;i<n;i++) {
		for (j=0;j<m;j++) {
			sprintf(tempstr1,"X%04dY%04d",i,j);
			tempstr1[10]=0;
			n1=agnode(result, tempstr1,true);

			sprintf(tempstr2,"X%04dY%04d",i,(j+1)%m);
			tempstr2[10]=0;
			n2=agnode(result, tempstr2,true);
			e=agedge(result,n1,n2,NULL,true);

			sprintf(tempstr2,"X%04dY%04d",i,(j-1+m)%m);
			tempstr2[10]=0;
			n2=agnode(result, tempstr2,true);
			e=agedge(result,n1,n2,NULL,true);

			sprintf(tempstr2,"X%04dY%04d",(i+1)%n,j);
			tempstr2[10]=0;
			n2=agnode(result, tempstr2,true);
			e=agedge(result,n1,n2,NULL,true);

			sprintf(tempstr2,"X%04dY%04d",(i-1+n)%n,j);
			tempstr2[10]=0;
			n2=agnode(result, tempstr2,true);
			e=agedge(result,n1,n2,NULL,true);
		}
	}
	return result;
}

Agraph_t * lattice1d (GVC_t * gvc, int open, int n)
{
	int i;

	Agraph_t * result=NULL;

	Agnode_t *n1, *n2;
	Agedge_t *e;

	char tempstr1[11];
	char tempstr2[11];

	result=agopen("dsgraph", Agdirected, NULL);

	for (i=0;i<n;i++) {
		sprintf(tempstr1,"X%04d",i);
		tempstr1[10]=0;
		n1=agnode(result, tempstr1,true);
	}

	for (i=0;i<n;i++) {
		sprintf(tempstr1,"X%04d",i);
		tempstr1[10]=0;
		n1=agnode(result, tempstr1,true);

		if (i==0) {
			if ((open==0)&&(n>2)) {
				sprintf(tempstr2,"X%04d",n-1);
				tempstr2[10]=0;
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2,NULL,true);
			}
		} else {
			sprintf(tempstr2,"X%04d",i-1);
			tempstr2[10]=0;
			n2=agnode(result, tempstr2,true);
			e=agedge(result,n1,n2,NULL,true);
		}

		if (i==n-1) {
			if ((open==0)&&(n>2)) {
				sprintf(tempstr2,"X%04d",0);
				tempstr2[10]=0;
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2,NULL,true);
			}
		} else {
			sprintf(tempstr2,"X%04d",i+1);
			tempstr2[10]=0;
			n2=agnode(result, tempstr2,true);
			e=agedge(result,n1,n2,NULL,true);
		}
	}
	return result;
}

Agraph_t * lattice2d(GVC_t* gvc, int open, int n, int m)
{
	int i,j;

	Agraph_t * result=NULL;

	Agnode_t *n1, *n2;
	Agedge_t *e;

	char tempstr1[11];
	char tempstr2[11];

	result=agopen("dsgraph", Agdirected, NULL);

	for (i=0;i<n;i++) {
		for (j=0;j<m;j++) {
			sprintf(tempstr1,"X%04dY%04d",i,j);
			tempstr1[10]=0;
			n1=agnode(result, tempstr1,true);
		}
	}

	for (i=0;i<n;i++) {
		for (j=0;j<m;j++) {
			sprintf(tempstr1,"X%04dY%04d",i,j);
			tempstr1[10]=0;
			n1=agnode(result, tempstr1,true);

			if (i==0) {
				if ((open==0)&&(n>2)) {
					sprintf(tempstr2,"X%04dY%04d",n-1,j);
					tempstr2[10]=0;
					n2=agnode(result, tempstr2,true);
					e=agedge(result,n1,n2,NULL,true);
				}
			} else {
				sprintf(tempstr2,"X%04dY%04d",i-1,j);
				tempstr2[10]=0;
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2,NULL,true);
			}

			if (i==n-1) {
				if ((open==0)&&(n>2)) {
					sprintf(tempstr2,"X%04dY%04d",0,j);
					tempstr2[10]=0;
					n2=agnode(result, tempstr2,true);
					e=agedge(result,n1,n2,NULL,true);
				}
			} else {
				sprintf(tempstr2,"X%04dY%04d",i+1,j);
				tempstr2[10]=0;
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2,NULL,true);
			}

			if (j==0) {
				if ((open==0)&&(m>2)) {
					sprintf(tempstr2,"X%04dY%04d",i,m-1);
					tempstr2[10]=0;
					n2=agnode(result, tempstr2,true);
					e=agedge(result,n1,n2,NULL,true);
				}
			} else {
				sprintf(tempstr2,"X%04dY%04d",i,j-1);
				tempstr2[10]=0;
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2,NULL,true);
			}

			if (j==m-1) {
				if ((open==0)&&(m>2)) {
					sprintf(tempstr2,"X%04dY%04d",i,0);
					tempstr2[10]=0;
					n2=agnode(result, tempstr2,true);
					e=agedge(result,n1,n2,NULL,true);
				}
			} else {
				sprintf(tempstr2,"X%04dY%04d",i,j+1);
				tempstr2[10]=0;
				n2=agnode(result, tempstr2,true);
				e=agedge(result,n1,n2,NULL,true);
			}
	
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

	// Graph type
	char * graph_type=NULL;

	// Graph parameters
	char * graph_parameters=NULL;

	// Graph(viz) managment
	GVC_t* gvc;
	Agraph_t * dsgraph;

	Agnode_t * inode;
	Agedge_t * iedge;

	// Start with the command line parsing
	while ((c = getopt (argc, argv, "hvVt:p:")) != -1)
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
		case 't':
			graph_type=strcpy((char *) malloc(strlen(optarg)+1),optarg);
			break;
		case 'p':
			graph_parameters=strcpy((char *) malloc(strlen(optarg)+1),optarg);
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

	if (graph_type==NULL || graph_parameters==NULL) {
		fprintf(stderr,"No graph type specified\n");
		gvFreeContext(gvc);
		exit(1);
	}

	if (!strcmp(graph_type,"hypercube")) {
		if (atoi(graph_parameters)<1 && atoi(graph_parameters)>6) {
			fprintf(stderr,"Hypercube dimension must be at least 1 and at most 6\n");
			gvFreeContext(gvc);
			exit(1);
		} else {
			i=atoi(graph_parameters);
			dsgraph=hypercube(gvc,i);
		}
	} else if (!strcmp(graph_type,"lattice1d")) {
		if (sscanf(graph_parameters,"%d,%d",&i,&j)!=2) {
			fprintf(stderr,"Lattice1d parameters must be in the form open,n\n");
			gvFreeContext(gvc);
			exit(1);
		}
		if ((i!=0)&&(i!=1) || (j<1) || (j>100)) {
			fprintf(stderr,"Lattice1d parameters must be in the form open,n with open=0 or 1 and n between 1 and 100\n");
			gvFreeContext(gvc);
			exit(1);
		}
		dsgraph=lattice1d(gvc,i,j);
	} else if (!strcmp(graph_type,"lattice2d")) {
		if (sscanf(graph_parameters,"%d,%d,%d",&i,&j,&k)!=3) {
			fprintf(stderr,"Lattice2d parameters must be in the form open,n,m\n");
			gvFreeContext(gvc);
			exit(1);
		}
		if ((i!=0)&&(i!=1) || (j<1) || (k<1) || (j>100) || (k>100)) {
			fprintf(stderr,"Lattice2d parameters must be in the form open,n,m with open=0 or 1 and n,m between 1 and 100\n");
			gvFreeContext(gvc);
			exit(1);
		}
		dsgraph=lattice2d(gvc,i,j,k);
	} else if (!strcmp(graph_type,"toroid")) {
		if (sscanf(graph_parameters,"%d,%d",&i,&j)!=2) {
			fprintf(stderr,"Toroid parameters must be in the form n,m\n");
			gvFreeContext(gvc);
			exit(1);
		}
		if ((i<1) || (j<1) || (i>100) || (j>100)) {
			fprintf(stderr,"Toroid parameters must be in the form n,m with n,m between 1 and 100\n");
			gvFreeContext(gvc);
			exit(1);
		}
		dsgraph=toroid(gvc,i,j);
	} else {
		fprintf(stderr,"Unknown graph type %s\n",graph_type);
		gvFreeContext(gvc);
		exit(1);
	}

	gvLayout (gvc, dsgraph,"dot");
	gvRender (gvc, dsgraph,"dot",stdout);
	gvFreeLayout(gvc,dsgraph);

	agclose (dsgraph);
	gvFreeContext(gvc);
}
