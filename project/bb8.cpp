// car.cpp
// implementazione dei metodi definiti in car.h

#include <stdio.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <vector> // la classe vector di STL

#include "bb8.h"

#include "point3.h"
#include "mesh.h"

// var globale di tipo mesh
Mesh head((char *)"obj/bb8-2-head-2.obj"); // chiama il costruttore
Mesh body((char *)"obj/bb8-2-body-2.obj");

//Mesh ship((char *)"BB8/Ship.obj");
//Mesh pista((char *)"pista.obj");

extern bool useEnvmap; // var globale esterna: per usare l'evnrionment mapping
extern bool useHeadlight; // var globale esterna: per usare i fari
extern bool useShadow; // var globale esterna: per generare l'ombra

// da invocare quando e' stato premuto/rilasciato il tasto numero "keycode"
void Controller::EatKey(int keycode, int* keymap, bool pressed_or_released){
  for (int i=0; i<NKEYS; i++)
    if (keycode==keymap[i]) key[i]=pressed_or_released;
}

// da invocare quando e' stato premuto/rilasciato jbutton"
void Controller::Joy(int keymap, bool pressed_or_released){
    key[keymap]=pressed_or_released;
}

bool isAccellerating(float newX,float oldX){
    if(fabs(newX)>fabs(oldX)){
        return true;
    }else{
        return false;
    }
}
// DoStep: facciamo un passo di fisica (a delta-t costante)
//
// Indipendente dal rendering.
//
// ricordiamoci che possiamo LEGGERE ma mai SCRIVERE
// la struttura controller da DoStep
void BB8::DoStep(){

    float vxm, vym, vzm; // velocita' in spazio macchina

    // da vel frame mondo a vel frame macchina
    float cosf = cos(facing*M_PI/180.0);
    float sinf = sin(facing*M_PI/180.0);
    vxm = +cosf*vx - sinf*vz;
    vzm = +sinf*vx + cosf*vz;

    if(is_falling){
        vym = -(0.5*9.81*t*t);
        vy = vym *attritoY;
        //printf("sta cadendo %f\n",vy);
    }else{
        if(is_jumping){
            vym = 20*sin(alpha_jump)*t - (0.5*9.81*t*t);// salto
        }else{
            vym = vy;
        }

        if(vym<0){
            vym=0;
            is_jumping = false;
        }
        // gestione dello sterzo
        if (controller.key[Controller::LEFT]){
            sterzo+=velSterzo;
            //vxm-= (accMax/3);
        }
        if (controller.key[Controller::RIGHT]){
            sterzo-=velSterzo;
            //vxm+=(accMax/3);
        }
        sterzo*=velRitornoSterzo; // ritorno a volante fermo

        if (controller.key[Controller::ACC]) vzm-=accMax;// accelerazione in avanti
        if (controller.key[Controller::DEC]) vzm+=accMax;// accelerazione indietro
        //printf("%f\n",vym);
        if (controller.key[Controller::JUMP]){
            if(!is_jumping){
                is_jumping = true;
                t=0;
            }
        }
        if(controller.key[Controller::ROTHEAD_RIGHT]) rotHead_torsione+=spin;
        if(controller.key[Controller::ROTHEAD_LEFT]) rotHead_torsione-=spin;
        rotHead_torsione*=0.90;

        // attriti (semplificando)
        vxm*=attritoX;
        vym*=attritoY;
        vzm*=attritoZ;

        // l'orientamento della macchina segue quello dello sterzo
        // (a seconda della velocita' sulla z)
        facing = facing - (vzm*grip)*sterzo;

        // rotazione mozzo ruote (a seconda della velocita' sulla z)
        float da,dh,dn; //delta angolo, delta head
        da=(360.0*vzm)/(2.0*M_PI*raggioRuotaA);
        mozzoA+=da;

        dh=(360.0*vxm)/(2.0*M_PI*raggioRuotaA);
        mozzoP+=dh;

        //printf("%f\n", rotHead);
        // ritorno a vel coord mondo
        vx = +cosf*vxm + sinf*vzm;
        vy = vym;
        vz = -sinf*vxm + cosf*vzm;
        dn = (spin*vzm)/(2.0*M_PI*raggioRuotaA);

        //printf("vx %f - vy %f - vz %f - dh %f\n", vx,vy,vz,dh);
        if(isAccellerating(vz,vz_old) || isAccellerating(vx,vx_old)){
            velRitornoHead = 0.99;
            if(rotHead<45 && rotHead>-45)
                rotHead -= dn;
        }else{
            rotHead += dn*1.5;
            rotHead *= velRitornoHead;
            velRitornoHead-=0.0002;
        }

        //printf("%f\n",rotHead);
        vx_old = vx;
        vy_old = vy;
        vz_old = vz;
    }

    px+=vx;
    py=vy;
    pz+=vz;
}

void Controller::Init(){
  for (int i=0; i<NKEYS; i++) key[i]=false;
}

void BB8::Init(){
  px=pz=facing=0; // posizione e orientamento
  py=0.0;

  mozzoA=mozzoP=rotHead=sterzo=0;   // stato
  vx=vy=vz=0;      // velocita' attuale
  vx_old=vy_old=vz_old=0.001;      // velocita' attuale
  rotHead_torsione=0;
  // inizializzo la struttura di controllo
  controller.Init();

  velSterzo=2.6;         // A
  velRitornoSterzo=0.99; // B, sterzo massimo = A*B / (1-B)

  velRitornoHead=0.999;
  velRitornoHead_bounce=1.05;
  spin = 18;
  accMax = 0.0015;
  t=0;
  alpha_jump=45;
  is_jumping=false;
  is_falling=false;
  // attriti: percentuale di velocita' che viene mantenuta
  // 1 = no attrito
  // <<1 = attrito grande
  attritoZ = 0.991;  // piccolo attrito sulla Z (nel senso di rotolamento delle ruote)
  attritoX = 0.991;  // grande attrito sulla X (per non fare slittare la macchina)
  attritoY = 0.981;  // attrito sulla y nullo

  // Nota: vel max = accMax*attritoZ / (1-attritoZ)

  raggioRuotaA = 0.45;
  raggioRuotaP = 0.35;

  grip = 0.45; // quanto il facing macchina si adegua velocemente allo sterzo
}

// disegna a schermo
void BB8::RenderAllParts(bool usecolor) const {
    // sono nello spazio mondo
    glEnable(GL_LIGHTING);
    glTranslatef(px,py,pz);
    glRotatef(facing, 0,1,0);
    glScalef(-0.5,0.5,-0.5);

    glEnable(GL_TEXTURE_2D);

    glPushMatrix();
        glColor3f(0.8,0.8,0.8);
        glTranslatef(0,body.bbmax.Y()+0.3,0); //aggiusto posizione per mash tagliata male
        glRotatef( -rotHead,1,0,0);
        glRotatef( sterzo/2,0,0,1);
        glRotatef( 90+rotHead_torsione,0,1,0);
        head.RenderNxV(8);
    glPopMatrix();

    //Body Render
    glPushMatrix();
        glColor3f(0.8,0.8,0.8);
        glTranslatef(0,body.bbmax.Y(),0);
        glRotatef( -sterzo,0,1,0);
        glRotatef( -mozzoA,1,0,0);
        glRotatef( -mozzoP,0,0,1);

        body.RenderNxV(6);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);


    //ombra
    glPushMatrix();
        glColor3f(0.31, 0.24, 0); // colore fisso
        glTranslatef(0,0.01-py,0); // alzo l'ombra di un epsilon per evitare z-fighting con il pavimento
        if(is_jumping){
            glScalef(0.50,0,0.50);  // appiattisco sulla Y, ingrandisco dell'1% sulla Z e sulla X
        }else{
            glScalef(0.95,0,0.95);  // appiattisco sulla Y, ingrandisco dell'1% sulla Z e sulla X
        }
        glDisable(GL_LIGHTING); // niente lighing per l'ombra
        body.RenderNxV(7); // disegno la macchina appiattita
        glRotatef( -rotHead,1,0,0);
        glRotatef( sterzo/2,0,0,1);
        head.RenderNxV(9);
        glEnable(GL_LIGHTING);
    glPopMatrix();
}
void BB8::Render() {
    if(is_jumping || is_falling){
        t+=0.1;
    }

    RenderAllParts(true);

}
