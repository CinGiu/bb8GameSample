#include <math.h>
#include <time.h>       /* time */
#include <stdlib.h>     /* srand, rand */
#include <string>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector> // la classe vector di STL

#include "point3.h"
#include "mesh.h"
#include "bb8.h"

#define CAMERA_BACK_CAR 0
#define CAMERA_TOP_FIXED 1
#define CAMERA_TOP_CAR 2
#define CAMERA_PILOT 3
#define CAMERA_MOUSE 4
#define CAMERA_TYPE_MAX 5
#define FLOOR_SIZE_MAX 100

GLfloat		xrot		=  0.0f;						// X Rotation
GLfloat		yrot		=  0.0f;						// Y Rotation
GLfloat		xrotspeed	=  0.0f;						// X Rotation Speed
GLfloat		yrotspeed	=  0.0f;						// Y Rotation Speed
GLfloat		zoom		= -7.0f;						// Depth Into The Screen
GLfloat		height		=  2.0f;						// Height Of Ball From Floor

float viewAlpha=20, viewBeta=40; // angoli che definiscono la vista
float eyeDist=5.0; // distanza dell'occhio dall'origine
int scrH=720, scrW=1080; // altezza e larghezza viewport (in pixels)
bool useWireframe=false;
bool useEnvmap=true;
bool useHeadlight=false;
bool useShadow=true;
int cameraType=0;
bool enableSolarSystem = 1;
float floor_max_x,floor_min_x,floor_max_z,floor_min_z=0;
BB8 bb8; // la nostra macchina
int nstep=0; // numero di passi di FISICA fatti fin'ora
const int PHYS_SAMPLING_STEP=10; // numero di millisec che un passo di fisica simula
int alpha_pista=0;
// Frames Per Seconds
const int fpsSampling = 3000; // lunghezza intervallo di calcolo fps
float fps=0; // valore di fps dell'intervallo precedente
int fpsNow=0; // quanti fotogrammi ho disegnato fin'ora nell'intervallo attuale
Uint32 timeLastInterval=0; // quando e' cominciato l'ultimo intervallo
int rand_path_size;
int score = 0;
GLvoid *font_style = GLUT_BITMAP_TIMES_ROMAN_10;
unsigned char* image_giulio = NULL;
Mesh saber((char *)"obj/LightSaber.obj");
Mesh moon((char *)"obj/moon.obj");
Mesh crate((char *)"obj/crate.obj");
//Mesh spaceman((char *)"obj/spaceman.obj");
int iheight, iwidth;
extern void drawPista(int path_size[]);
bool sword[4]= {true,true,true,true};
int intorno = 5;
float rivoluzione_moon = 0;
float rotazione_moon = 0;
bool menu = false;
bool isMirror = false;
// setta le matrici di trasformazione in modo
// che le coordinate in spazio oggetto siano le coord
// del pixel sullo schemo
void  SetCoordToPixel(){
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-1,-1,0);
    glScalef(2.0/scrW, 2.0/scrH, 1);
}

bool LoadTexture(int textbind,char *filename){
  SDL_Surface *s = IMG_Load(filename);
  if (!s) return false;

  glBindTexture(GL_TEXTURE_2D, textbind);
  gluBuild2DMipmaps(
        GL_TEXTURE_2D,
        GL_RGB,
        s->w, s->h,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        s->pixels
  );
  glTexParameteri(
      GL_TEXTURE_2D,
      GL_TEXTURE_MAG_FILTER,
      GL_LINEAR );
  glTexParameteri(
      GL_TEXTURE_2D,
      GL_TEXTURE_MIN_FILTER,
      GL_LINEAR_MIPMAP_LINEAR );

  return true;
}


void drawSphere(double r, int lats, int longs) {
    int i, j;
    for(i = 0; i <= lats; i++) {
        double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
        double z0  = sin(lat0);
        double zr0 =  cos(lat0);

        double lat1 = M_PI * (-0.5 + (double) i / lats);
        double z1 = sin(lat1);
        double zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);

        for(j = 0; j <= longs; j++) {
            double lng = 2 * M_PI * (double) (j - 1) / longs;
            double x = cos(lng);
            double y = sin(lng);

            //le normali servono per l'EnvMap
            glNormal3f(x * zr0, y * zr0, z0);
            glVertex3f(r * x * zr0, r * y * zr0, r * z0);
            glNormal3f(x * zr1, y * zr1, z1);
            glVertex3f(r * x * zr1, r * y * zr1, r * z1);
        }
        glEnd();
    }
}

void drawSolarSystem(){
    if(enableSolarSystem){
        glDisable(GL_LIGHTING);
        glPushMatrix();
            glTranslatef(0,0,0); // 3. Translate to the object's position.
            glRotatef(-rotazione_moon,1,0.0,1); // 2. Rotate the object.
            glTranslatef(floor_max_x+100,0,0);
            glRotatef( -rivoluzione_moon ,0,1,0);
                glScalef(0.4,0.4,0.4);
                glColor3f(1,1,0);
                moon.RenderNxV(-1);
                float tmpv[4] = {0,10,0,  1}; // ultima comp=0 => luce direzionale
                glLightfv(GL_LIGHT0, GL_POSITION, tmpv );
        glPopMatrix();
        glEnable(GL_LIGHTING);
        glPushMatrix();
            glTranslatef(0,0,0); // 3. Translate to the object's position.
            glRotatef(-rotazione_moon,1,0.0,1); // 2. Rotate the object.
            glTranslatef(0,0,floor_min_z+100);
            glRotatef( -rivoluzione_moon ,0,1,0);
            glScalef(0.5,0.5,0.5);
                moon.RenderNxV(16);
        glPopMatrix();
    }else{
        float tmpv[4] = {0,15,0,  1}; // ultima comp=0 => luce direzionale
        glLightfv(GL_LIGHT0, GL_POSITION, tmpv );


    }

}
void drawCubeFill(int s, int l,int p){
    int h=-1;
    const float S=l; // size
    const float H=0;   // altezza
    const int K=s; //disegna K x K quads
    glBindTexture(GL_TEXTURE_2D,12);
    glBegin(GL_QUADS);

      //glColor3f(0.6, 0.6, 0.6); // colore uguale x tutti i quads
      glNormal3f(0,1,0);       // normale verticale uguale x tutti
      for (int x=0; x<K; x++)
      for (int z=0; z<K; z++) {
        float x0=-S + 2*(x+0)*S/K;
        float x1=-S + 2*(x+1)*S/K;
        float z0=-S + 2*(z+0)*S/K;
        float z1=-S + 2*(z+1)*S/K;
        glTexCoord2f(0.0,0.0);
        glVertex3d(x0, H, z0);
        glTexCoord2f(1.0,0.0);
        glVertex3d(x1, H, z0);
        glTexCoord2f(1.0,1.0);
        glVertex3d(x1, H, z1);
        glTexCoord2f(0.0,1.0);
        glVertex3d(x0, H, z1);
      }
    glEnd();
    /*  bordi del pavimento*/
    floor_max_x = +p+l;
    floor_max_z = +p-s;
    floor_min_x = +p-l;
    floor_min_z = +p+s;

    //printf("%f - %f - %f - %f\n", floor_max_x, floor_min_x, floor_max_z,floor_min_z);
}
void drawFloor(){

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_COLOR_MATERIAL);
    drawCubeFill(rand_path_size,rand_path_size,0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glEnable(GL_LIGHTING);
        glPushMatrix();
            glTranslatef(floor_max_x-2,2,floor_max_z+4);
            glScalef(4,4,4);
            crate.RenderNxV(15);

        glPopMatrix();
        glPushMatrix();
            glTranslatef(floor_min_x+2,2,floor_min_z);
            glScalef(4,4,4);
            crate.RenderNxV(15);
        glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    if(sword[0]){
      glPushMatrix() ;
          glTranslatef(floor_min_x+2,5,floor_min_z-2);
          glScalef(0.1,0.1,0.1);
          glColor3f(1,1,0.5);
              saber.RenderNxV(-1);

      glPopMatrix();
    }

    if(sword[1]){
      glPushMatrix();
          glColor3f(0,0.7,0);
          glTranslatef(floor_max_x,0.5,floor_min_z);
          glScalef(0.1,0.1,0.1);
              saber.RenderNxV(-1);
      glPopMatrix();
    }
    if(sword[2]){
      glPushMatrix();
          glColor3f(0.5,0.8,0.8);
          glTranslatef(floor_min_x,0.5,floor_max_z);
          glScalef(0.1,0.1,0.1);
              saber.RenderNxV(-1);
      glPopMatrix();
    }
    if(sword[3]){
      glPushMatrix();
          glColor3f(1,1,1);
          glTranslatef(floor_max_x-2,5,floor_max_z+2);
          glScalef(0.1,0.1,0.1);
              saber.RenderNxV(-1);
      glPopMatrix();
    }
}

// setto la posizione della camera
void setCamera(){

        double px = bb8.px;
        double py = bb8.py;
        double pz = bb8.pz;
        double angle = bb8.facing;
        double cosf = cos(angle*M_PI/180.0);
        double sinf = sin(angle*M_PI/180.0);
        double camd, camh, ex, ey, ez, cx, cy, cz;
        double cosff, sinff;

// controllo la posizione della camera a seconda dell'opzione selezionata
        switch (cameraType) {
        case CAMERA_BACK_CAR:
                camd = 19.5;
                camh = 5.0;
                ex = px + camd*sinf;
                ey = py + camh;
                ez = pz + camd*cosf;
                cx = px - camd*sinf;
                cy = py + camh;
                cz = pz - camd*cosf;
                gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                break;
        case CAMERA_TOP_FIXED:
                camd = 12.5;
                camh = 5.0;
                angle = bb8.facing + 40.0;
                cosff = cos(angle*M_PI/180.0);
                sinff = sin(angle*M_PI/180.0);
                ex = px + camd*sinff;
                ey = py + camh;
                ez = pz + camd*cosff;
                cx = px - camd*sinf;
                cy = py + camh;
                cz = pz - camd*cosf;
                gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                break;
        case CAMERA_TOP_CAR:
                camd = 12.5;
                camh = 5.0;
                ex = px + camd*sinf;
                ey = py + camh;
                ez = pz + camd*cosf;
                cx = px - camd*sinf;
                cy = py + camh;
                cz = pz - camd*cosf;
                gluLookAt(ex,ey+5,ez,cx,cy,cz,0.0,1.0,0.0);
                break;
        case CAMERA_PILOT:
                camd = 12.5;
                camh = 5.0;
                ex = px + camd*sinf;
                ey = py + camh;
                ez = pz + camd*cosf;
                cx = px - camd*sinf;
                cy = py + camh;
                cz = pz - camd*cosf;
                gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                break;
        case CAMERA_MOUSE:
                glTranslatef(0,0,-eyeDist);
                glRotatef(viewBeta,  1,0,0);
                glRotatef(viewAlpha, 0,1,0);
                break;
        }
}

void setfont(char const* name, int size){
    font_style = GLUT_BITMAP_HELVETICA_10;
    if (strcmp(name, "helvetica") == 0) {
        if (size == 12)
            font_style = GLUT_BITMAP_HELVETICA_12;
        else if (size == 18)
            font_style = GLUT_BITMAP_HELVETICA_18;
    } else if (strcmp(name, "times roman") == 0) {
        font_style = GLUT_BITMAP_TIMES_ROMAN_10;
        if (size == 24)
            font_style = GLUT_BITMAP_TIMES_ROMAN_24;
    } else if (strcmp(name, "8x13") == 0) {
        font_style = GLUT_BITMAP_8_BY_13;
    } else if (strcmp(name, "9x15") == 0) {
        font_style = GLUT_BITMAP_9_BY_15;
    }
}

void drawstr(GLuint x, GLuint y, char const* format, ...){
    va_list args;
    char buffer[255], *s;

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    glRasterPos2i(x, y);
    for (s = buffer; *s; s++)
        glutBitmapCharacter(font_style, *s);
}

void drawSky() {
    int H = 100;

    if (useWireframe) {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0,0,0);
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

        drawSphere(100.0, 20, 20);

        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        glColor3f(1,1,1);
        glEnable(GL_LIGHTING);
    }else{
        glBindTexture(GL_TEXTURE_2D,2);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);

        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);

        glColor3f(1,1,1);
        glDisable(GL_LIGHTING);

        //   drawCubeFill();
        drawSphere(500.0, 50, 50);

        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
    }
}

void drawImgGiulio(){
    glBindTexture(GL_TEXTURE_2D,4);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_LIGHTING);

    //MURO CON FOTO
    glNormal3f(0,0,1);       // normale verticale uguale x tutti

    glBegin(GL_QUADS);
        glTexCoord2f(0.0,0.0);
        glVertex3f( -15,30,-80);
        glTexCoord2f(1.0,0.0);
        glVertex3f( +15,30,-80);
        glTexCoord2f(1.0,1.0);
        glVertex3f( +15,0,-80);
        glTexCoord2f(0.0,1.0);
        glVertex3f( -15,0,-80);

    glEnd();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
        glColor3f(1,0,0);
        glTranslatef(-14,0,-80);
        glScalef(0.3,0.3,0.3);
            saber.RenderNxV(10);
    glPopMatrix();
    glPushMatrix();
        glColor3f(0,0,1);
        glTranslatef(14,0,-80);
        glScalef(0.3,0.3,0.3);
            saber.RenderNxV(10);
    glPopMatrix();

}
void drawMenu(){
    if(menu){
        char buff[50];
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);

            glColor4f(1, 0.6, 0,0.9);
            glVertex2d( 0,scrH-30);
            glVertex2d( 0,0);

            glVertex2d( scrW/3,0);
            glVertex2d( scrW/3,scrH-30);
        glEnd();
        glDisable(GL_BLEND);

        glColor3f(0.3,0.3,0.3);
        setfont("times roman", 24);
        int line = 50;
        sprintf(buff,"-Menu-");
        drawstr(scrW/9, scrH-54, buff);
        sprintf(buff,"Comandi");
        drawstr(scrW/9, scrH-80, buff);

        sprintf(buff,"WASD - Movimento");
        drawstr(10, scrH-(line*3), buff);

        sprintf(buff,"SPACE - Salto");
        drawstr(10, scrH-(line*4), buff);

        sprintf(buff,"Q E - Torsione testa");
        drawstr(10, scrH-(line*5), buff);

        sprintf(buff,"Z - Abilita/Disabilita sistema solare");
        drawstr(10, scrH-(line*6), buff);
        sprintf(buff,"F1 - Cambia visuale");
        drawstr(10, scrH-(line*7), buff);

        sprintf(buff,"F2 - Cambia texture/wireframe");
        drawstr(10, scrH-(line*8), buff);

        sprintf(buff,"P - Attiva il portale");
        drawstr(10, scrH-(line*9), buff);


        sprintf(buff,"-OBIETTIVO-");
        drawstr(scrW/9-40, scrH-(line*10), buff);

        sprintf(buff,"Cattura tutte le spade senza cadere!");
        drawstr(10, scrH-(line*11), buff);

        sprintf(buff,"m - Chiudi menu");
        drawstr(10, scrH-(line*14), buff);
    }
}
void drawScore(){
    char buff[50];
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
        glColor4f(1, 0.6, 0.01,0.8);
        glVertex2d( 0,scrH);
        glVertex2d( 0,scrH-30);

        glVertex2d( scrW,scrH-30);
        glVertex2d( scrW,scrH);
    glEnd();
    glDisable(GL_BLEND);
    glColor3f(0,0,0);
    setfont("times roman", 24);
    sprintf(buff,"Punti: %d",score);
    drawstr(10, scrH-24, buff);
    sprintf(buff,"PREMI m per il MENU");
    drawstr(scrW-250, scrH-24, buff);

    int size = rand_path_size*4;
    int center = size/2;
    //glScalef(4,4,0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
        glColor4f(0.1, 0.1, 0.1, 0.9);
        glVertex2d( scrW-0,0);
        glVertex2d( scrW-0,size);
        glVertex2d( scrW-size,size);
        glVertex2d( scrW-size,0);
    glEnd();
    glDisable(GL_BLEND);
    int size_quad = 8;

    glBegin(GL_QUADS);
        glColor3f(1, 0, 0);
        glVertex2d( scrW-center+(bb8.px*2)+size_quad,center-bb8.pz*2-size_quad);
        glVertex2d( scrW-center+(bb8.px*2)-size_quad,center-bb8.pz*2-size_quad);
        glVertex2d( scrW-center+(bb8.px*2)-size_quad,center-bb8.pz*2+size_quad);
        glVertex2d( scrW-center+(bb8.px*2)+size_quad,center-bb8.pz*2+size_quad);
    glEnd();
    int size_sword = 7;
    /*Spade*/
    if(sword[0]){
        glBegin(GL_QUADS);
            glColor3f(1,1,0.5);
            glVertex2d( scrW-size,0);
            glVertex2d( scrW-size,size_sword);
            glVertex2d( scrW-size+size_sword,size_sword);
            glVertex2d( scrW-size+size_sword,0);
        glEnd();
    }
    if(sword[1]){
        glBegin(GL_QUADS);
            glColor3f(0,0.7,0);
            glVertex2d( scrW,0);
            glVertex2d( scrW,size_sword);
            glVertex2d( scrW-size_sword,size_sword);
            glVertex2d( scrW-size_sword,0);
        glEnd();
    }
    if(sword[2]){
        glBegin(GL_QUADS);
            glColor3f(0.5,0.8,0.8);

            glVertex2d( scrW-size,size);
            glVertex2d( scrW-size,size-size_sword);
            glVertex2d( scrW-size+size_sword,size-size_sword);
            glVertex2d( scrW-size+size_sword,size);
        glEnd();
    }
    if(sword[3]){
        glBegin(GL_QUADS);
            glColor3f(1,1,1);
            glVertex2d( scrW,size);
            glVertex2d( scrW,size-size_sword);
            glVertex2d( scrW-size_sword,size-size_sword);
            glVertex2d( scrW-size_sword,size);
        glEnd();
    }

    if(!sword[0] && !sword[1] && !sword[2] && !sword[3]){
        //fine gioco
        if(score>0){

            drawstr((scrW/2)-10, scrH-20, "Fine. Hai vinto!");
        }else{
            drawstr((scrW/2)-10, scrH-20, "Fine. perso!");
        }
    }


    //printf("facing: %f\n",facing);

}

void drawMirror(){
    if(isMirror){
        glPushMatrix();
        glTranslatef(floor_min_x,0,0);
        glBindTexture(GL_TEXTURE_2D,18);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);

        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR); // Env map
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
        float s[3] = {0,0,0.06};
        float t[3] = {0,0.06,0};
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_REPEAT );
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT );
            glTexGenfv(GL_S, GL_OBJECT_PLANE , s);
            glTexGenfv(GL_T, GL_OBJECT_PLANE , t);
            glNormal3f(0,0,0);       // normale verticale uguale x tutti
            glColor3f(1,1,1);
            glBegin(GL_QUADS);
                glVertex3f( 0,12,7);
                glVertex3f( 0,12,9);
                glVertex3f( 0,0,9);
                glVertex3f( 0,0,7);
            glEnd();

            glBegin(GL_QUADS);
                glVertex3f( 0,12,-7);
                glVertex3f( 0,12,-9);
                glVertex3f( 0,0,-9);
                glVertex3f( 0,0,-7);
            glEnd();
            glBegin(GL_QUADS);
                glVertex3f( 0,10,-9);
                glVertex3f( 0,10,9);
                glVertex3f( 0,12,9);
                glVertex3f( 0,12,-9);
            glEnd();
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();

        glClear(GL_STENCIL_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        /* Draw 1 into the stencil buffer. */
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xffffffff);

        glPushMatrix();
        glTranslatef(floor_min_x,0,0);
            glNormal3f(1,0,0);       // normale verticale uguale x tutti
            glColor3f(1,1,1);
            glBegin(GL_QUADS);
                glVertex3f( 0,10,+7);
                glVertex3f( 0,10,-7);
                glVertex3f( 0,0,-7);
                glVertex3f( 0,0,+7);
            glEnd();



            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glEnable(GL_DEPTH_TEST);

            glStencilFunc(GL_EQUAL, 1, 0xffffffff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glTranslatef(floor_min_x,0,0);
            glScalef(-1.0, 1.0, 1.0);
            glEnable(GL_NORMALIZE);
            //glCullFace(GL_FRONT);
            drawFloor();

            drawSolarSystem();
            drawImgGiulio();
            bb8.Render();
            glDisable(GL_NORMALIZE);
            //glCullFace(GL_BACK);

        glPopMatrix();
        glDisable(GL_STENCIL_TEST);
    }

}
/* Esegue il Rendering della scena */
void rendering(SDL_Window *win){

  // un frame in piu'!!!
  fpsNow++;

  glLineWidth(3); // linee larghe

  // settiamo il viewport
  glViewport(0,0, scrW, scrH);
  //glViewport(0,0, 100, 100);

  // colore sfondo = bianco
  glClearColor(1,1,1,1);


  // settiamo la matrice di proiezione
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluPerspective( 70, //fovy,
		((float)scrW) / scrH,//aspect Y/X,
		0.2,//distanza del NEAR CLIPPING PLANE in coordinate vista
		1000  //distanza del FAR CLIPPING PLANE in coordinate vista
  );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  // Clear Screen, Depth Buffer & Stencil Buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


  setCamera();



  drawSky(); // disegna il cielo come sfondo

  rivoluzione_moon+=0.6;
  rotazione_moon+=0.4;
  drawSolarSystem();
  drawFloor(); // disegna il suolo

  drawImgGiulio();
drawMirror();
  bb8.Render();


    //cameraType = CAMERA_TOP_FIXED;
    //setCamera();

    //cameraType = 0;
  // attendiamo la fine della rasterizzazione di
  // tutte le primitive mandate

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  SetCoordToPixel();

  drawScore();
  //menu = true;
  drawMenu();

  // disegnamo i fps (frame x sec) come una barra a sinistra.
  // (vuota = 0 fps, piena = 100 fps)
  glBegin(GL_QUADS);
  float y=scrH*fps/100;
  float ramp=fps/100;
  glColor3f(1-ramp,0,ramp);
  glVertex2d(10,0);
  glVertex2d(10,y);
  glVertex2d(0,y);
  glVertex2d(0,0);
  glEnd();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);


  //glScalef(0.1,0.1,0.1);
  glFinish();
  // ho finito: buffer di lavoro diventa visibile
  SDL_GL_SwapWindow(win);
  glFlush ();
}

void redraw(){
  // ci automandiamo un messaggio che (s.o. permettendo)
  // ci fara' ridisegnare la finestra
  SDL_Event e;
  e.type=SDL_WINDOWEVENT;
  e.window.event=SDL_WINDOWEVENT_EXPOSED;
  SDL_PushEvent(&e);
}

int main(int argc, char* argv[]){
    SDL_Window *win;
    SDL_GLContext mainContext;
    Uint32 windowID;
    SDL_Joystick *joystick;
    static int keymap[Controller::NKEYS] = {SDLK_a, SDLK_d, SDLK_w, SDLK_s, SDLK_SPACE, SDLK_q, SDLK_e};

    //glutInit
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);
    glutInitWindowSize(scrW, scrH);
    glutInitWindowPosition(0, 0);
    // inizializzazione di SDL
    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    SDL_JoystickEventState(SDL_ENABLE);
    joystick = SDL_JoystickOpen(0);

    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
    SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 24 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    // facciamo una finestra di scrW x scrH pixels
    win=SDL_CreateWindow(argv[0], 0, 0, scrW, scrH, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    //Create our opengl context and attach it to our window

    mainContext=SDL_GL_CreateContext(win);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_NORMALIZE);

    glFrontFace(GL_CW); // consideriamo Front Facing le facce ClockWise
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
    glEnable(GL_POLYGON_OFFSET_FILL); // caro openGL sposta i
                                    // frammenti generati dalla
                                    // rasterizzazione poligoni
    glPolygonOffset(1,1);             // indietro di 1

    if (!LoadTexture(2,(char *)"img/star_sky.png")) return 0;
    if (!LoadTexture(4,(char *)"img/giulio_cinelli.jpg")) return 0;
    if (!LoadTexture(6,(char *)"img/BODY_MAP.jpg")) return 0;
    if (!LoadTexture(8,(char *)"img/HEAD_MAP.jpg")) return 0;
    if (!LoadTexture(10,(char *)"img/sabre.png")) return 0;
    if (!LoadTexture(12,(char *)"img/sand.jpg")) return 0;
    if (!LoadTexture(15,(char *)"img/crate_tex3.jpg")) return 0;
    if (!LoadTexture(16,(char *)"img/moonmap2k.jpg")) return 0;
    if (!LoadTexture(18,(char *)"img/portal.jpg")) return 0;


    /*init random path lenght*/

    //rand_path_size = rand() % 40 + 30;
    rand_path_size = 30;


    bool done=0;
    glutInit(&argc, argv);

    while (!done) {

        SDL_Event e;

        // guardo se c'e' un evento:
        if (SDL_PollEvent(&e)) {
         // se si: processa evento
            switch (e.type) {
                case SDL_KEYDOWN:
                    bb8.controller.EatKey(e.key.keysym.sym, keymap , true);
                    if (e.key.keysym.sym==SDLK_F1) cameraType=(cameraType+1)%CAMERA_TYPE_MAX;
                    if (e.key.keysym.sym==SDLK_F2) useWireframe=!useWireframe;
                    if (e.key.keysym.sym==SDLK_F3) useEnvmap=!useEnvmap;
                    if (e.key.keysym.sym==SDLK_F4) useHeadlight=!useHeadlight;
                    if (e.key.keysym.sym==SDLK_F5) useShadow=!useShadow;
                    if (e.key.keysym.sym==SDLK_m) menu=!menu;
                    if (e.key.keysym.sym==SDLK_p) isMirror=!isMirror;
                    if (e.key.keysym.sym==SDLK_z) enableSolarSystem=!enableSolarSystem;
                    if (e.key.keysym.sym==SDLK_l) rand_path_size=(rand_path_size+20)%FLOOR_SIZE_MAX;

                    break;
                case SDL_KEYUP:
                    bb8.controller.EatKey(e.key.keysym.sym, keymap , false);
                    break;
                case SDL_QUIT:
                    done=1;   break;
                case SDL_WINDOWEVENT:
                 // dobbiamo ridisegnare la finestra
                 if (e.window.event==SDL_WINDOWEVENT_EXPOSED)
                    rendering(win);
                 else{
                     windowID = SDL_GetWindowID(win);
                     if (e.window.windowID == windowID)  {
                         switch (e.window.event)  {
                             case SDL_WINDOWEVENT_SIZE_CHANGED:  {
                                 scrW = e.window.data1;
                                 scrH = e.window.data2;
                                 glViewport(0,0,scrW,scrH);
                                 rendering(win);
                                 //redraw(); // richiedi ridisegno
                                 break;
                             }
                         }
                     }
                 }
                 break;

                 case SDL_MOUSEMOTION:
                     if (e.motion.state & SDL_BUTTON(1) & cameraType==CAMERA_MOUSE) {
                         viewAlpha+=e.motion.xrel;
                         viewBeta +=e.motion.yrel;
                         //          if (viewBeta<-90) viewBeta=-90;
                         if (viewBeta<+5) viewBeta=+5; //per non andare sotto la macchina
                         if (viewBeta>+90) viewBeta=+90;
                         // redraw(); // richiedi un ridisegno (non c'e' bisongo: si ridisegna gia'
                         // al ritmo delle computazioni fisiche)
                     }
                     break;

                 case SDL_MOUSEWHEEL:

                     if (e.wheel.y < 0 ) {
                         //printf("culo\n");
                         // avvicino il punto di vista (zoom in)
                         eyeDist=eyeDist*0.9;
                         if (eyeDist<1) eyeDist = 1;
                     };
                     if (e.wheel.y > 0 ) {
                         // allontano il punto di vista (zoom out)
                         eyeDist=eyeDist/0.9;
                     };
                     break;

                case SDL_JOYAXISMOTION: /* Handle Joystick Motion */
                    if( e.jaxis.axis == 0){
                        if ( e.jaxis.value < -3200  )
                        {
                            bb8.controller.Joy(0 , true);
                            bb8.controller.Joy(1 , false);
                            //	      printf("%d <-3200 \n",e.jaxis.value);
                        }
                        if ( e.jaxis.value > 3200  )
                        {
                            bb8.controller.Joy(0 , false);
                            bb8.controller.Joy(1 , true);
                            //	      printf("%d >3200 \n",e.jaxis.value);
                        }
                        if ( e.jaxis.value >= -3200 && e.jaxis.value <= 3200 )
                        {
                            bb8.controller.Joy(0 , false);
                            bb8.controller.Joy(1 , false);
                            //	      printf("%d in [-3200,3200] \n",e.jaxis.value);
                        }

                        //redraw();
                    }
                    break;
                case SDL_JOYBUTTONDOWN: /* Handle Joystick Button Presses */
                    if ( e.jbutton.button == 0 ){
                       bb8.controller.Joy(2 , true);
                    //	   printf("jbutton 0\n");
                    }
                    if ( e.jbutton.button == 2 ){
                       bb8.controller.Joy(3 , true);
                    //	   printf("jbutton 2\n");
                    }
                    break;
                case SDL_JOYBUTTONUP: /* Handle Joystick Button Presses */
                   bb8.controller.Joy(2 , false);
                   bb8.controller.Joy(3 , false);
                break;
            }
        } else {
              // nessun evento: siamo IDLE

            Uint32 timeNow=SDL_GetTicks(); // che ore sono?

            if (timeLastInterval+fpsSampling<timeNow) {
                fps = 1000.0*((float)fpsNow) /(timeNow-timeLastInterval);
                fpsNow=0;
                timeLastInterval = timeNow;
            }

            bool doneSomething=false;
            int guardia=0; // sicurezza da loop infinito

            // finche' il tempo simulato e' rimasto indietro rispetto
            // al tempo reale...
            while (nstep*PHYS_SAMPLING_STEP < timeNow ) {
                bb8.DoStep();
                nstep++;
                doneSomething=true;
                timeNow=SDL_GetTicks();
                if (guardia++>1000) {done=true; break;} // siamo troppo lenti!
            }
            if (doneSomething){
                //Dentro il perimetro
                if(bb8.px > floor_max_x || bb8.px < floor_min_x || bb8.pz < floor_max_z || bb8.pz > floor_min_z){
                    bb8.is_falling = true;
                    bb8.is_jumping = true;
                }
                //caduta in basso e ripristino
                if(bb8.py < -250){
                    bb8.is_falling = false;
                    bb8.t=0;
                    bb8.py = 0;
                    bb8.px = 0;
                    bb8.pz = 0;
                    score -= 10;
                }
                //printf("%f %f\n", bb8.pz,bb8.px);
                // Cattura spade
                if((bb8.px < (floor_min_x + intorno+2)) && (bb8.pz > (floor_min_z - intorno-2) && (bb8.py >= 2))){
                    if(sword[0]){
                        sword[0] = false;
                        score += 5;
                    }
                }
                if((bb8.px > (floor_max_x - intorno)) && (bb8.pz > (floor_min_z - intorno))){
                    if(sword[1]){
                        sword[1] = false;
                        score += 5;
                    }
                }
                if((bb8.px < (floor_min_x + intorno)) && (bb8.pz < (floor_max_z + intorno))){
                    if(sword[2]){
                        sword[2] = false;
                        score += 5;
                    }
                }
                if((bb8.px > (floor_max_x-2 - intorno)) && (bb8.pz < (floor_max_z+4 + intorno) && (bb8.py >= 2))){
                    if(sword[3]){
                        sword[3] = false;
                        score += 5;
                    }
                }
                if(isMirror){
                    if(bb8.px <= (floor_min_x) && (bb8.pz < 7) && (bb8.pz > -7)){
                        //printf("PORTALE" );
                        bb8.px = floor_max_x;
                        bb8.is_falling = false;
                        bb8.is_jumping = false;
                    }
                }

                rendering(win);
            }
            else {
            // tempo libero!!!
            }
        }
    }
SDL_GL_DeleteContext(mainContext);
SDL_DestroyWindow(win);
SDL_Quit ();
return (0);
}
