#include <graphviz/gvc.h>
#include <graphviz/graph.h>

int main(int argc, char **argv)
{
	Agnode_t * curr;

	graph_t * g;
	FILE *fp;

	if (argc > 1) {
		fp=fopen(argv[1],"r");
		if (!fp) {
			exit(0);
		}
	} else {
		exit(0);
	}

	aginit();

	g=agread(fp);

	for (curr=agfstnode(g);curr!=NULL;curr=agnxtnode(g,curr))
	printf("%s\n",curr->name);

	agclose(g);
	return (1);
}
