#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

#include "ksource.h"


void KS_error(const char* msg);

Metric* Metric_create(int dim, const double* scaling, PerturbFun perturb, int nps, const double* params){
	Metric* metric = (Metric*)malloc(sizeof(Metric));
	int i;
	metric->dim = dim;
	metric->scaling = (float*)malloc(dim*sizeof(float));
	if(scaling) for(i=0; i<dim; i++) metric->scaling[i] = (float)scaling[i];
	else for(i=0; i<dim; i++) metric->scaling[i] = 0;
	metric->perturb = perturb;
	metric->nps = nps;
	metric->params = (double*)malloc(nps * sizeof(double));
	for(i=0; i<nps; i++) metric->params[i] = params[i];
	return metric;
}

Metric* Metric_copy(const Metric* from){
	Metric* metric = (Metric*)malloc(sizeof(Metric));
	*metric = *from;
	int i;
	metric->scaling = (float*)malloc(metric->dim*sizeof(double));
	for(i=0; i<metric->dim; i++) metric->scaling[i] = from->scaling[i];
	metric->params = (double*)malloc(metric->nps * sizeof(double));
	for(i=0; i<metric->nps; i++) metric->params[i] = from->params[i];
	return metric;
}

void Metric_destroy(Metric* metric){
	free(metric->scaling);
	free(metric);
}

Geometry* Geom_create(int ord, Metric** metrics, double bw, const char* bwfilename,
		const double* trasl, const double* rot){
	Geometry* geom = (Geometry*)malloc(sizeof(Geometry));
	geom->ord = ord;
	int i;
	geom->ms = (Metric**)malloc(ord * sizeof(Metric*));
	for(i=0; i<ord; i++) geom->ms[i] = metrics[i];
	geom->bw = bw;
	geom->bwfilename = NULL;
	geom->bwfile = NULL;
	if(bwfilename) if(strlen(bwfilename)){
		FILE* bwfile;
		if((bwfile=fopen(bwfilename, "r")) == 0){
			printf("No se pudo abrir archivo %s\n", bwfilename);
			KS_error("Error en Geom_create");
		}
		geom->bwfilename = (char*)malloc(NAME_MAX_LEN*sizeof(char));
		strcpy(geom->bwfilename, bwfilename);
		geom->bwfile = bwfile;
		Geom_next(geom);
	}
	if(trasl){
		geom->trasl = (double*)malloc(3 * sizeof(double));
		for(i=0; i<3; i++) geom->trasl[i] = trasl[i];
	}
	else geom->trasl = NULL;
	if(rot){
		geom->rot = (double*)malloc(3 * sizeof(double));
		for(i=0; i<3; i++) geom->rot[i] = rot[i];
	}
	else geom->rot = NULL;
	return geom;
}

Geometry* Geom_copy(const Geometry* from){
	Geometry* geom = (Geometry*)malloc(sizeof(Geometry));
	*geom = *from;
	int i;
	geom->ms = (Metric**)malloc(geom->ord * sizeof(Metric*));
	for(i=0; i<geom->ord; i++) geom->ms[i] = Metric_copy(from->ms[i]);
	geom->bwfilename = NULL;
	geom->bwfile = NULL;
	if(geom->bwfile){
		geom->bwfilename = (char*)malloc(NAME_MAX_LEN*sizeof(char));
		strcpy(geom->bwfilename, from->bwfilename);
		geom->bwfile = fopen(geom->bwfilename, "r");
		Geom_next(geom);
	}
	if(from->trasl){
		geom->trasl = (double*)malloc(3 * sizeof(double));
		for(i=0; i<3; i++) geom->trasl[i] = from->trasl[i];
	}
	else geom->trasl = NULL;
	if(from->rot){
		geom->rot = (double*)malloc(3 * sizeof(double));
		for(i=0; i<3; i++) geom->rot[i] = from->rot[i];
	}
	else geom->rot = NULL;
	return geom;
}

int Geom_perturb(const Geometry* geom, mcpl_particle_t* part){
	int i, ret=0;
	if(geom->trasl) traslv(part->position, geom->trasl, 1);
	if(geom->rot){ rotv(part->position, geom->rot, 1); rotv(part->direction, geom->rot, 1); }
	for(i=0; i<geom->ord; i++)
		ret += geom->ms[i]->perturb(geom->ms[i], part, geom->bw);
	if(geom->rot){ rotv(part->position, geom->rot, 0); rotv(part->direction, geom->rot, 0); }
	if(geom->trasl) traslv(part->position, geom->trasl, 0);
	return ret;
}

int Geom_next(Geometry* geom){
	if(geom->bwfile){
		int i, readed=0, count=0;
		while(readed!=1 && count++<2){ // Intento leer 2 veces
			if(count == 2) rewind(geom->bwfile); // Antes del 2do intento rebobino
			readed = fread(&geom->bw, sizeof(float), 1, geom->bwfile);
		}
		if(count == 3)
			KS_error("Error en Geom_next: No se pudo leer ancho de banda");
	}
	return 0;
}

void Geom_destroy(Geometry* geom){
	int i;
	for(i=0; i<geom->ord; i++) Metric_destroy(geom->ms[i]);
	free(geom->ms);
	free(geom->trasl);
	free(geom->rot);
	free(geom);
}


int E_perturb(const Metric* metric, mcpl_particle_t* part, double bw){
	part->ekin += bw*metric->scaling[0] * rand_norm();
	if(part->ekin < E_MIN) part->ekin = E_MIN;
	if(part->ekin > E_MAX) part->ekin = E_MAX;
	return 0;
}
int Let_perturb(const Metric* metric, mcpl_particle_t* part, double bw){
	part->ekin *= exp(bw*metric->scaling[0] * rand_norm());
	if(part->ekin < E_MIN) part->ekin = E_MIN;
	if(part->ekin > E_MAX) part->ekin = E_MAX;
	return 0;
}

int Vol_perturb(const Metric* metric, mcpl_particle_t* part, double bw){
	part->position[0] += bw*metric->scaling[0] * rand_norm();
	part->position[1] += bw*metric->scaling[1] * rand_norm();
	part->position[2] += bw*metric->scaling[2] * rand_norm();
	if(part->position[0] < metric->params[0]) part->position[0] = metric->params[0];
	else if(part->position[0] > metric->params[1]) part->position[0] = metric->params[1];
	if(part->position[1] < metric->params[2]) part->position[1] = metric->params[2];
	else if(part->position[1] > metric->params[3]) part->position[1] = metric->params[3];
	if(part->position[2] < metric->params[4]) part->position[2] = metric->params[4];
	else if(part->position[2] > metric->params[5]) part->position[2] = metric->params[5];
	return 0;
}
int SurfXY_perturb(const Metric* metric, mcpl_particle_t* part, double bw){
	part->position[0] += bw*metric->scaling[0] * rand_norm();
	part->position[1] += bw*metric->scaling[1] * rand_norm();
	if(part->position[0] < metric->params[0]) part->position[0] = metric->params[0];
	else if(part->position[0] > metric->params[1]) part->position[0] = metric->params[1];
	if(part->position[1] < metric->params[2]) part->position[1] = metric->params[2];
	else if(part->position[1] > metric->params[3]) part->position[1] = metric->params[3];
	return 0;
}
int Guide_perturb(const Metric* metric, mcpl_particle_t* part, double bw){
	double x=part->position[0], y=part->position[1], z=part->position[2], dx=part->direction[0], dy=part->direction[1], dz=part->direction[2];
	double xwidth=metric->params[0], yheight=metric->params[1], zmax=metric->params[2], rcurv=metric->params[3];
	double t, theta, phi, theta0, dx2, dz2;
	int cont=0, mirror;
	if(rcurv != 0){ // Transformar a variables de guia curva
		double r = sqrt((rcurv+x)*(rcurv+x) + z*z);
		x = copysign(1, rcurv) * r - rcurv; z = fabs(rcurv) * asin(z / r);
		dx2 = dx; dz2 = dz; dx = dx2*cos(z/rcurv) + dz2*sin(z/rcurv); dz = -dx2*sin(z/rcurv) + dz2*cos(z/rcurv);
	}
	// Transformar de (x,y,z,dx,dy,dz) a (z,t,theta,phi)
	if((y/yheight > -x/xwidth) && (y/yheight <  x/xwidth))      mirror=0; // espejo x pos
	else if((y/yheight >  x/xwidth) && (y/yheight > -x/xwidth)) mirror=1; // espejo y pos
	else if((y/yheight < -x/xwidth) && (y/yheight >  x/xwidth)) mirror=2; // espejo x neg
	else                                                        mirror=3; // espejo y neg
	switch(mirror){
		case 0:
			t = 0.5*yheight + y;
			theta0 = acos(dx); phi = atan2(-dy, dz);
			break;
		case 1:
			t = 1.0*yheight + 0.5*xwidth - x;
			theta0 = acos(dy); phi = atan2(dx, dz);
			break;
		case 2:
			t = 1.5*yheight + 1.0*xwidth - y;
			theta0 = acos(-dx); phi = atan2(dy, dz);
			break;
		case 3:
			t = 2.0*yheight + 1.5*xwidth + x;
			theta0 = acos(-dy); phi = atan2(-dx, dz);
			break;
	}
	// Perturbar
	z += bw*metric->scaling[0] * rand_norm();
	t += bw*metric->scaling[1] * rand_norm();
	theta = theta0 + bw*metric->scaling[2]*M_PI/180 * rand_norm();
	while(cos(theta0)*cos(theta) < 0){ // Evitar que perturbacion cambie sentido de propagacion
		theta = theta0 + bw*metric->scaling[2]*M_PI/180 * rand_norm();
		if(cont++ == MAX_RESAMPLES){
			printf("Warning en Polar_perturb: MAX_RESAMPLES alcanzado\n");
			break;
		}
	}
	phi += bw*metric->scaling[3]*M_PI/180 * rand_norm();
	// Aplicar restricciones a perturbaciones
	if(z < 0) z = 0;
	else if(z > zmax) z = zmax;
	while(t < 0) t += 2*(xwidth+yheight);
	while(t > 2*(xwidth+yheight)) t -= 2*(xwidth+yheight);
	switch(mirror){
		case 0: if(t<  0)              t=  0;              else if(t>  yheight)          t=  yheight;          break;
		case 1: if(t<  yheight)        t=  yheight;        else if(t>  yheight+  xwidth) t=  yheight+  xwidth; break;
		case 2: if(t<  yheight+xwidth) t=  yheight+xwidth; else if(t>2*yheight+  xwidth) t=2*yheight+  xwidth; break;
		case 3: if(t<2*yheight+xwidth) t=2*yheight+xwidth; else if(t>2*yheight+2*xwidth) t=2*yheight+2*xwidth; break;
	}
	// Antitransformar de (z,t,theta_n,theta_t) a (x,y,z,dx,dy,dz)
	switch(mirror){
		case 0:
			x =  xwidth/2; y =  t - 0.5*yheight;
			dx = cos(theta); dz = sin(theta)*cos(phi); dy = -sin(theta)*sin(phi);
			break;
		case 1:
			y =  yheight/2; x = -t + yheight + 0.5*xwidth;
			dy = cos(theta); dz = sin(theta)*cos(phi); dx = sin(theta)*sin(phi);
			break;
		case 2:
			x = -xwidth/2; y = -t + 1.5*yheight + xwidth;
			dx = -cos(theta); dz = sin(theta)*cos(phi); dy = sin(theta)*sin(phi);
			break;
		case 3:
			y = -yheight/2; x =  t - 2*yheight - 1.5*xwidth;
			dy = -cos(theta); dz = sin(theta)*cos(phi); dx = -sin(theta)*sin(phi);
			break;
	}
	if(rcurv != 0){ // Antitransformar de variables de guia curva
		double r = (rcurv + x) * copysign(1, rcurv), ang = z / rcurv;
		x = copysign(1, rcurv) * r * cos(ang) - rcurv; z = r * sin(fabs(ang));
		dx2 = dx; dz2 = dz; dx = dx2*cos(ang) - dz2*sin(ang); dz = dx2*sin(ang) + dz2*cos(ang);
	}
	part->position[0] = x; part->position[1] = y; part->position[2] = z;
	part->direction[0] = dx; part->direction[1] = dy; part->direction[2] = dz;
	return 0;
}

int Isotrop_perturb(const Metric* metric, mcpl_particle_t* part, double bw){
	if(bw*metric->scaling[0] == INFINITY){
		part->direction[2] = -1 + 2.*rand()/RAND_MAX;
		double dxy = sqrt(1-part->direction[2]*part->direction[2]);
		double phi = 2.*M_PI*rand()/RAND_MAX;
		part->direction[0] = dxy*cos(phi);
		part->direction[1] = dxy*sin(phi);
	}
	else if(bw*metric->scaling[0] > 0){
		double xi = (double)rand()/RAND_MAX;
		double w = 1;
		w += bw*metric->scaling[0]*bw*metric->scaling[0] * log(xi+(1-xi)*exp(-2/(bw*metric->scaling[0]*bw*metric->scaling[0])));
		double phi = 2.*M_PI*rand()/RAND_MAX;
		double uv = sqrt(1-w*w), u = uv*cos(phi), v = uv*sin(phi);
		double x=part->direction[0], y=part->direction[1], z=part->direction[2];
		if(part->direction[2] > 0){
			part->direction[0] = u*z + w*x - (v*x-u*y)*y/(1+z);
			part->direction[1] = v*z + w*y + (v*x-u*y)*x/(1+z);
		}
		else{
			part->direction[0] = u*z + w*x + (v*x-u*y)*y/(1-z);
			part->direction[1] = v*z + w*y - (v*x-u*y)*x/(1-z);
		}
		part->direction[2] = w*z - u*x - v*y;
	}
	return 0;
}

int Polar_perturb(const Metric* metric, mcpl_particle_t* part, double bw){
	double theta, phi, theta0;
	int cont=0;
	theta0 = acos(part->direction[2]);
	phi   = atan2(part->direction[1], part->direction[0]);
	theta = theta0 + bw*metric->scaling[0]*M_PI/180 * rand_norm();
	while(cos(theta0)*cos(theta) < 0){ // Evitar que perturbacion cambie sentido de propagacion
		theta = theta0 + bw*metric->scaling[0]*M_PI/180 * rand_norm();
		if(cont++ == MAX_RESAMPLES){
			printf("Warning en Polar_perturb: MAX_RESAMPLES alcanzado\n");
			break;
		}
	}
	phi   += bw*metric->scaling[1]*M_PI/180 * rand_norm();
	part->direction[0] = sin(theta) * cos(phi);
	part->direction[1] = sin(theta) * sin(phi);
	part->direction[2] = cos(theta);
	return 0;
}
