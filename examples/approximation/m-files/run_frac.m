%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
alpha = 1.0;
s   = -0.5;
beta  = 0.0;
t   =  0.5;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
lev =  10;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
dim=2;
%% load matrix
disp('Loading matrix A...')
load A.dat
nrowa=A(1,1); ncola=A(1,2); nnza=A(1,3); A=A(2:nnza+1,1:3);
A=sparse(A(:,1)+1,A(:,2)+1,A(:,3),nrowa,ncola);
disp('Loading matrix M...')
load M.dat
nrowm=M(1,1); ncolm=M(1,2); nnzm=M(1,3); M=M(2:nnzm+1,1:3);
M=sparse(M(:,1)+1,M(:,2)+1,M(:,3),nrowm,ncolm);
disp('Loading matrix b and u...')
load b.dat ; b=b(2:end);
load u.dat ; u=u(2:end);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
sm=dim*(dim+1)/min(diag(M)); sa=1/norm(A,inf);
sm=1;
sa=1;
bnd0=0;
bnd1=max(eig(sa*A,sm*M));%% sm*sa;
status0=system('make -C ..');
comm0=sprintf('../aaa.ex <<EOF_FRAC >../m-files/frac.m\n %.2Lf %.2Lf %.2Lf %.2Lf %.2f %.2f\nEOF_FRAC\n',s,t,alpha,beta,bnd0,bnd1);
%%      disp(comm0)
status1=system(comm0)
[res,pol,z,w,f,er]=frac();
m=length(res);
m1=m-1;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
ff=((sm/sa)^s)*M*b;
asf=res(m)*ff;
for j=1:m1
    asf=asf+res(j)*((sa*A-sm*pol(j)*M)\ff);
end
asf=M*asf;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

