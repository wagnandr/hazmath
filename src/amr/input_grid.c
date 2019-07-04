/*! \file src/amr/input_grid.c
 *
 *  Created by James Adler, Xiaozhe Hu, and Ludmil Zikatanov on 20190420.
 *  Copyright 2019__HAZMATH__. All rights reserved.
 *
 *  \note containing all essential routines for input for the mesh
 *  generation mesh refinement
 *
 */
#include "hazmath.h"
/**********************************************************************/
void input_grid_free(input_grid *g)
{
  if(g->title) free(g->title);
  if(g->dgrid) free(g->dgrid);
  if(g->fgrid) free(g->fgrid);
  if(g->dvtu) free(g->dvtu);
  if(g->fvtu) free(g->fvtu);
  if(g->ox) free(g->ox);
  if(g->systypes) free(g->systypes);
  if(g->syslabels) free(g->syslabels);
  if(g->csysv) free(g->csysv);
  if(g->bcodesv) free(g->bcodesv);
  if(g->xv) free(g->xv);
  if(g->xe) free(g->xe);
  if(g->seg) free(g->seg);
  if(g) free(g);
  return;
}
/**********************************************************************/
void input_grid_print(input_grid *g)
{
  // prints input grid. 
  INT i,j,dim=g->dim;
  fprintf(stdout,"\n\nTITLE: %s",g->title);
  fprintf(stdout,"\ndimension=%d",g->dim);
  fprintf(stdout,"\nprint_level=%d",g->print_level);
  fprintf(stdout,"\ndir_grid=%s",g->dgrid);
  fprintf(stdout,"\ndir_vtu=%s",g->dvtu);
  fprintf(stdout,"\nfile_grid=%s",g->fgrid);
  fprintf(stdout,"\nfile_vtu=%s",g->fvtu);
  fprintf(stdout,"\n\nnum_coordsystems=%d",g->ncsys);
  for(i=0;i<g->ncsys;i++){
    fprintf(stdout,"\nlabel=%d,type=%d, origin(",g->syslabels[i],g->systypes[i]);
    for(j=0;j<g->dim;j++) fprintf(stdout," %6.2f ",g->ox[i*dim+j]);
    fprintf(stdout,")");
  }
  fprintf(stdout,"\n\nnum_vertices=%d",g->nv);
  for(i=0;i<g->nv;i++){
    fprintf(stdout,"\nvertex=%d, coord_system=%d, bcode=%d, coords(",i,g->csysv[i],g->bcodesv[i]);
    if(g->systypes[g->csysv[i]]==1){
      fprintf(stdout," %6.2f ",g->xv[i*dim]);
      for(j=1;j<g->dim;j++)	
	fprintf(stdout," %6.2f ",(g->xv[i*dim+j])/((REAL )PI)*180.);
    }else{
      for(j=0;j<g->dim;j++) fprintf(stdout," %6.2f ",g->xv[i*dim+j]);
    }
    fprintf(stdout,")");
  }
  fprintf(stdout,"\n\nnum_edges=%d\n",g->ne);
  for(i=0;i<g->ne;i++){
    fprintf(stdout,"\nedge=(%d,%d) div=%d",g->seg[3*i],g->seg[3*i+1],g->seg[3*i+2]);
  }
  fprintf(stdout,"\n\n");fflush(stdout);
  
  return;
}
/* /\*---------------------------------------------------------------------*\/ */
/* void coo2csr(INT nrow,INT ncol,INT nnz,					\ */
/* 	     INT *row_idx,INT *col_idx, void *aval,			\ */
/* 	     INT *ia,INT *ja, void *bval,				\ */
/* 	     size_t elsize) */
/* { */
/*   // converts (i,j,value) to (ia,ja,bval) for matrices matrices whose */
/*   // elements have elsize bytes */
/*   // */
/*   // get dimensions of the matrix */
/*   // */
/*   const INT m=nrow, n=ncol; */
/*   INT i, iind, jind;   */
/*   INT *ind = (INT *) calloc(m+1,sizeof(INT)); */
/*   // initialize */
/*   memset(ind, 0, sizeof(INT)*(m+1));     */
/*   // count number of nonzeros in each row */
/*   for (i=0; i<nnz; ++i) ind[row_idx[i]+1]++;     */
/*   // set row pointer */
/*   ia[0] = 0; */
/*   for (i=1; i<=m; ++i) { */
/*     ia[i] = ia[i-1]+ind[i]; */
/*     ind[i] = ia[i]; */
/*   }     */
/*   // set column index and values */
/*   for (i=0; i<nnz; ++i) { */
/*     iind = row_idx[i]; */
/*     jind = ind[iind]; */
/*     ja[jind] = col_idx[i]; */
/*     memcpy((bval+jind*elsize),(aval+i*elsize),elsize); */
/*     ind[iind] = ++jind; */
/*   }     */
/*   if (ind) free(ind);     */
/*   return; */
/* } */
/*---------------------------------------------------------------------*/
char **splits(char *s, const char *d, INT *num)
{
  INT k;
  char **w=calloc(strlen(s)+1,sizeof(char *));
  char *work=strtok(s,d);
  k=0;
  while(work!= NULL){	
    w[k]=calloc(strlen(work)+1,sizeof(char));
    memcpy(w[k],work,(strlen(work)+1)*sizeof(char)); // +1 to copy \0
						     // at the end;
    work = strtok(NULL,d);
    k++;
  }
  w[k]=NULL;
  *num=k;
  return w;
}
/*==========================================================*/
void read_data(char *data_coordsystems,		\
	       char *data_vertices,		\
	       char *data_edges,		\
	       input_grid *g)
{
  // read first the data related to coord systems. 
  char **w;
  INT i,iread,count,k,j,num;
  w=splits(data_coordsystems," ",&num);
  k=0;
  for(count=0;count<g->ncsys;count++){
    if(w[k]==NULL) break;
    iread=sscanf(w[k],"%d",&g->syslabels[count]);
    if(iread<0) iread=0;
    free(w[k]);
    k++;
    for(j=0;j<g->dim;j++){
      iread=sscanf(w[k],"%lg",(g->ox +count*g->dim+j));
      if(iread<0) iread=0;
      free(w[k]);
      k++;
    }
    iread=sscanf(w[k],"%d",&g->systypes[count]);
    if(iread<0) iread=0;
    free(w[k]);
    k++;
  }
  if(w) free(w);
  /***** vertices *****/
  w=splits(data_vertices," ",&num);
  k=0;
  for(count=0;count<g->nv;count++){
    if(w[k]==NULL) break;
    for(j=0;j<g->dim;j++){
      iread=sscanf(w[k],"%lg",(g->xv + count*g->dim+j));
      if(iread<0) iread=0;
      if(w[k]) free(w[k]);
      k++;
    }
    iread=sscanf(w[k],"%d",&g->csysv[count]);
    if(iread<0) iread=0;
    if(w[k]) free(w[k]);
    k++;
    iread=sscanf(w[k],"%d",&g->bcodesv[count]);
    if(iread<0) iread=0;
    if(w[k]) free(w[k]);
    k++;
  }
  if(w) free(w);
  for(count=0;count<g->nv;count++){
    if(g->systypes[g->csysv[count]]==1){
      for(j=1;j<g->dim;j++){
	//	fprintf(stdout,"\n(%d%d) x=%f",count,j,g->xv[count*g->dim + j]);
	g->xv[count*g->dim + j]*=(((REAL )PI)/180.);
      }
    }
  }
  /***** edges *****/
  w=splits(data_edges," ",&num);
  k=0;
  for(count=0;count<g->ne;count++){
    if(w[k]==NULL) break;
    iread=sscanf(w[k],"%d",g->seg+3*count);
    if(iread<0) iread=0;
    if(w[k]) free(w[k]);
    k++;
    iread=sscanf(w[k],"%d",g->seg+3*count+1);
    if(iread<0) iread=0;
    if(w[k]) free(w[k]);
    k++;
    iread=sscanf(w[k],"%d",g->seg+3*count+2);
    if(iread<0) iread=0;
    if(w[k]) free(w[k]);
    k++;
  }
  INT ne = 0,iri,ici,ndd;
  // no selfedges
  for(i=0;i<g->ne;i++){
    iri=g->seg[3*i];ici=g->seg[3*i+1];ndd=g->seg[3*i+2];
    if(iri==ici) continue;
    if(iri<ici){
      g->seg[3*ne]=iri;
      g->seg[3*ne+1]=ici;
    } else {
      g->seg[3*ne]=ici;
      g->seg[3*ne+1]=iri;
    }
    g->seg[3*ne+2]=ndd;
    ne++;
  }
  if(ne<g->ne)
    g->seg=realloc(g->seg,3*ne*sizeof(INT));
  g->ne=ne;  
  if(w) free(w);
  return;
}
/********************************************************************/
void get_out(const char *pattern, size_t le)
{
  /* prints a string cutting it at the closest blank space <= le*/
  int i;
  fprintf(stderr, "\n\n\n           *** ERROR::::   \n     UNBALANCED \"{}\" near:::   ");
  for (i=0;i<le;++i)
    fprintf(stderr, "%c",*(pattern+i));
  fprintf(stderr, "...}\n");
  exit(128);
}
/***********************************************************************/
char *make_string_from_file(FILE *the_file, size_t *length_string)
{
  char *everything;
  char ch, ch_next;
  int count=0,i,j,flag;
  while(feof(the_file)==0) 
    {
      ch = fgetc(the_file);
      if(ch) count++;
    }
  count--;
  i = count*sizeof(char);
  everything = (char *)malloc(i);
  rewind(the_file);
  //  fprintf(stderr,"\nNumber of characters in the file %i\n", count);
  i = 0;
  j = 0;
  flag = 1;
  while(j < count) {
    if( flag ) {
      ch = fgetc(the_file);
      ++j;
    }
    if( ch == '%' ){
      /* delete to the end of line or to the end of file, whatever it is */
      do{
	ch = fgetc(the_file);
	++j;
      }  while(ch != '\n' && ch != EOF);
      if( ch == '\n' ) 
	flag=1;
      else 
	break; 
    } else {
      if(ch == '\n' || ch == '\t') ch = ' ';
      if(ch == ' ' || ch == '{' || ch ==  '}'){
	do{
	  ch_next = fgetc(the_file);
	  if(ch_next == '\n' || ch_next == '\t') ch_next = ' ';
	  ++j;
	}  while(ch_next == ' ');
	if(ch == ' ' && (ch_next ==  '{' || ch_next ==  '}')){
	  ch = ch_next;
	  flag=0;
	} else {
	  /*		  printf(" %i ",ch);*/
	  *(everything+i) = ch;
	  ++i;
	  ch = ch_next;
	  flag=0;
	}
      } else if( ch != EOF ) {
	/*	      printf(" %i ",ch);*/
	*(everything+i) = ch;
	++i;
	flag=1;
      }
    }
    /*           printf("\n i,j, %i %i  %i\n",i,j,j-count);*/
  }
  if(i) 
    if (*(everything+i-1)== ' ') i--;
  /* end of string */
  *(everything+i) = '\0';
  *length_string = i;
  everything = (char *)realloc(everything,i*sizeof(char));
  /*  fprintf(stderr,"\nNumber of characters in the supressed string
      %i\n", i); */
  /* fprintf(stderr,"\nNumber of characters in the supressed string
     %li\n",strlen(everything)); */
  return everything;
}
/********************************************************************/
char *get_substring(const char *pattern,		\
		    size_t *length_substring,	\
		    char *the_string)
{
  /* 
     get substring from a string matching a pattern; it can probably
     be done with strtok()
  */
  
  size_t le;
  INT i;
  char *found, *wrk;
  //  char ch;
  le = strlen(pattern);
  found = (char *) strstr(the_string,pattern);
  if(found != NULL){
    found = &(*(found + le));
    wrk = (char *) strstr(found,"}");
    if(wrk == NULL ){ get_out(pattern,le);}
    *length_substring=strlen(found)-strlen(wrk);
    *wrk = '\t';
    wrk = (char *) strstr(found,"\t");
    i = strlen(found)-strlen(wrk);
    if(i != *length_substring ){ get_out(pattern,le); }
  }else{
    fprintf(stderr, "\n\n\n *** WARNING:::: " );
    for (i=0;i<le;++i)
      fprintf(stderr, "%c",*(pattern+i));
    fprintf(stderr, "...} has not been found in the input file\n\n");
    found = (char *)malloc(sizeof(char));
    *length_substring=0;
  }
  return found;
}
input_grid *parse_input_grid(const char *input_file_grid)
{
  INT iread;
  FILE *the_file;
  char *everything;
  size_t length_string=0;
  char *title=NULL, 
    *dimension=NULL, 
    *print_level=NULL, 
    *dir_grid=NULL,
    *dir_vtu=NULL,
    *file_grid=NULL,
    *file_vtu=NULL,
    *num_coordsystems=NULL, 
    *data_coordsystems=NULL,
    *num_vertices=NULL,
    *data_vertices=NULL,
    *num_edges=NULL,
    *data_edges=NULL;
  //  size_t length_info_file;
  size_t length_title=0, 
    length_dimension=0, 
    length_print_level=0, 
    length_dir_grid=0,
    length_dir_vtu=0,
    length_file_grid=0,
    length_file_vtu=0,
    length_num_coordsystems=0, 
    length_data_coordsystems=0,
    length_num_vertices=0,
    length_data_vertices=0,
    length_num_edges=0,
    length_data_edges=0;
  the_file = fopen(input_file_grid,"r");
  everything = make_string_from_file(the_file, &length_string);
  fclose(the_file);
  //  fprintf(stdout,"\n%s\n",everything);
  /* get all substrings */
  title  = get_substring("title{",&length_title, everything);
  dimension  = get_substring("dimension{",&length_dimension, everything);
  print_level        = get_substring("print_level{",&length_print_level, everything);
  dir_grid      = get_substring("dir_grid{",   &length_dir_grid, everything);
  dir_vtu       = get_substring("dir_vtu{",&length_dir_vtu, everything);
  file_grid      = get_substring("file_grid{",&length_file_grid, everything);
  file_vtu         = get_substring("file_vtu{",&length_file_vtu, everything);
  num_coordsystems       = get_substring("num_coordsystems{",&length_num_coordsystems, everything);
  data_coordsystems       = get_substring("data_coordsystems{",&length_data_coordsystems, everything);
  num_vertices        = get_substring("num_vertices{",&length_num_vertices, everything);
  data_vertices     = get_substring("data_vertices{",&length_data_vertices, everything);
  num_edges  = get_substring("num_edges{",&length_num_edges, everything);
  data_edges = get_substring("data_edges{",&length_data_edges, everything);
  /* ... */
  //  fprintf(stdout,"\n***^^^TITLE:%s\n\n",title);fflush(stdout);
  *(title + length_title) = '\0';
  *(dimension + length_dimension) = '\0';
  *(print_level + length_print_level) = '\0';
  *(dir_grid + length_dir_grid) = '\0';
  *(dir_vtu + length_dir_vtu) = '\0';
  *(file_grid + length_file_grid) = '\0';
  *(file_vtu + length_file_vtu) = '\0';
  *(num_coordsystems + length_num_coordsystems) = '\0';
  *(data_coordsystems + length_data_coordsystems) = '\0';
  *(num_vertices + length_num_vertices) = '\0';
  *(data_vertices + length_data_vertices) = '\0';
  *(num_edges + length_num_edges) = '\0';
  *(data_edges + length_data_edges) = '\0';
  /* ... PARSE ... */
  input_grid *g=malloc(1*sizeof(input_grid));
  iread=sscanf(dimension,"%d",&g->dim); // the dimension of the problem.
  iread=sscanf(print_level,"%hd",&g->print_level);//
  if(iread<0) iread=0;
  g->title=(char *)calloc(strlen(title),sizeof(char));
  g->dgrid=(char *)calloc(strlen(dir_grid),sizeof(char));
  g->fgrid=(char *)calloc(strlen(file_grid),sizeof(char));
  g->dvtu=(char *)calloc(strlen(dir_vtu),sizeof(char));
  g->fvtu=(char *)calloc(strlen(file_vtu),sizeof(char));
  strncpy(g->title,title,strlen(title));  /**< grid dir name */
  strncpy(g->dgrid,dir_grid,strlen(dir_grid));  /**< grid dir name */
  strncpy(g->fgrid,file_grid,strlen(file_grid));  /**< grid file name */
  strncpy(g->dvtu,dir_vtu,strlen(dir_vtu));  /**< vtu grid */
  strncpy(g->fvtu,file_vtu,strlen(file_vtu));  /**< vtu file */
  // integers;
  iread=sscanf(num_coordsystems,"%d",&g->ncsys);//
  iread=sscanf(num_vertices,"%d",&g->nv);//
  iread=sscanf(num_edges,"%d",&g->ne);//
  if(iread<0) iread=0;
  //mixed
  g->ox=(REAL *)calloc(g->dim*g->ncsys,sizeof(REAL));
  g->systypes=(INT *)calloc(g->ncsys,sizeof(INT)); 
  g->syslabels=(INT *)calloc(g->ncsys,sizeof(INT)); 
  g->csysv=(INT *)calloc(g->nv,sizeof(INT)); 
  g->bcodesv=(INT *)calloc(g->nv,sizeof(INT)); 
  g->xv=(REAL *)calloc(g->dim*g->nv,sizeof(REAL)); 
  g->xe=(REAL *)calloc(g->dim*g->ne,sizeof(REAL)); 
  g->seg=(INT *)calloc(3*g->ne,sizeof(INT)); 
  /*  iCSRmat *graph;// icsrmat thing for the macroelement graph. */
  /* read arrays and convert data */
  read_data(data_coordsystems,data_vertices, data_edges,g);
  if(everything)free(everything);
  return g;
}
/********************************************************************/
