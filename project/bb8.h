class Controller{
public:
  enum { LEFT=0, RIGHT=1, ACC=2, DEC=3, JUMP=4, ROTHEAD_RIGHT=5, ROTHEAD_LEFT = 6, NKEYS=7};
  bool key[NKEYS];
  void Init();
  void EatKey(int keycode, int* keymap, bool pressed_or_released);
  void Joy(int keymap, bool pressed_or_released);
  Controller(){Init();} // costruttore
};

class BB8{
     void RenderAllParts(bool usecolor) const;
public:
  // Metodi
  void Init(); // inizializza variabili
  void Render(); // disegna a schermo
  void DoStep(); // computa un passo del motore fisico

  BB8(){Init();} // costruttore

  Controller controller;
  
  float px,py,pz,facing; // posizione e orientamento
  float mozzoA, mozzoP, rotHead, sterzo, alpha_jump; // stato interno
  float vx,vy,vz; // velocita' attuale
  float vx_old,vy_old,vz_old;
  float rotHead_torsione;
  bool is_jumping;
  bool is_falling;

  // STATS DELLA MACCHINA
  // (di solito rimangono costanti)
  float velSterzo, velRitornoSterzo, velRitornoHead, velRitornoHead_bounce, spin, accMax, attrito,
        raggioRuotaA, raggioRuotaP, grip, t, attritoX, attritoY, attritoZ; // attrit
};
