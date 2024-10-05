#include <inttypes.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/time.h>

double now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + tv.tv_usec/1000000.;
}

using namespace std;

static constexpr uint64_t rozmnoznik(uint64_t x){
  return x  | (x<<5)  | (x<<10) | (x<<15) |
    (x<<20) | (x<<25) | (x<<30) | (x<<35) |
    (x<<40) | (x<<45) | (x<<50) | (x<<55);
}

static constexpr uint64_t przygotowanie_pod_przeniesienie = rozmnoznik(22);
static constexpr uint64_t ekstraktor_bitow_przeniesienia = rozmnoznik(16);
static constexpr uint64_t czysciciel_bitow_przeniesienia = rozmnoznik(15);

struct liczba_10
{
  uint64_t schowek;
  liczba_10() {
    schowek = 0;
  }
  liczba_10(uint64_t v) {
    schowek = 0;
    for (size_t i = 0; i < 12; i++) {
      schowek = schowek >> 5;
      uint64_t r = v % 10;
      v = v / 10;
      schowek = schowek | (r << 55);
    }
  }
  uint32_t przeniesienie() const {
    return (schowek >> (5 * 12)) & 1;
  }
  void normalizuj_po_dodaniu() {
    uint64_t x;
    x = schowek & ekstraktor_bitow_przeniesienia;
    x >>= 3;
    x *= 3; //duplikator bitow
    schowek -= x;
    schowek &= czysciciel_bitow_przeniesienia;
  }
  std::string print() const {
    char buffer[13];
    char* pos = &buffer[13];
    uint64_t v = schowek;
    do
    {
      uint8_t r = v &0xf;
      pos--;
      *pos = '0' + r;
      v = v >> 5;
    } while ( v != 0);
    return std::string(pos, &buffer[13] - pos);
  }
};
liczba_10 dodaj(liczba_10 a, liczba_10 b) {
  liczba_10 res;
  res.schowek = a.schowek;
  res.schowek += przygotowanie_pod_przeniesienie;
  res.schowek += b.schowek;
  return res;
}



struct multiliczba_10
{
  static uint32_t wstazki; // ustawiane odpowiednio do ilosci cyfr
  static void ustaw_wstazke(uint32_t w) {
    assert(w < max_wstazka);
    wstazki = w;
  }
  static constexpr size_t max_wstazka = 8;
  liczba_10 wstazka[max_wstazka];
  multiliczba_10() {
  };
  multiliczba_10(liczba_10 a) {
    wstazka[0] = a;
  }
  multiliczba_10(const multiliczba_10& a) {
    for (uint32_t i = 0; i < wstazki; i++) {
      wstazka[i] = a.wstazka[i];
    }
  }
  multiliczba_10& operator+=(const multiliczba_10& a) {
    uint32_t p = 0;
    for (uint32_t i = 0; i < wstazki; i++) {
      wstazka[i] = dodaj(wstazka[i], a.wstazka[i]);
      wstazka[i].schowek += p;
      p = wstazka[i].przeniesienie();
      wstazka[i].normalizuj_po_dodaniu();
    }
    assert(p == 0);
    return *this;
  }
  uint8_t cyfra(uint8_t n) const {
    uint8_t li = n / 12;
    uint8_t pos = n % 12;
    uint64_t v = wstazka[li].schowek;
    v = v >> pos * 5;
    return v & 0xf;
  }
  
  multiliczba_10& operator*=(uint8_t n);
  string print() const {
    string s;
    int16_t p = wstazki * 12 - 1;
    while (p>0 && cyfra(p)==0) {
      p--;
    }
    while (p>=0) {
      s.push_back('0' + cyfra(p));
      p--;
    }
    return s;
  }
  bool jest_zerem() const {
    for (uint32_t i = 0; i < wstazki; i++) {
      if (wstazka[i].schowek != 0) return false;
    }
    return true;
  }
};
uint32_t multiliczba_10::wstazki;

multiliczba_10 operator+(const multiliczba_10& a, const multiliczba_10& b) {
  multiliczba_10 res(a);
  res+=b;
  return res;
}

multiliczba_10 operator*(multiliczba_10 a, uint8_t n) {
  multiliczba_10 r;
  while (n != 0) {
    if (n & 1) {
      r += a;
    }
    a += a;
    n = n >> 1;
  }
  return r;
}

multiliczba_10& multiliczba_10::operator*=(uint8_t n) {
  *this = *this * n;
  return *this;
}

bool operator<(const multiliczba_10& lewo, const multiliczba_10& prawo) {
  uint32_t i = multiliczba_10::wstazki - 1;
  do {
    if (lewo.wstazka[i].schowek < prawo.wstazka[i].schowek) return true;
    if (lewo.wstazka[i].schowek > prawo.wstazka[i].schowek) return false;
    if (i==0) return false;
    i--;
  } while (true);
}

bool operator<=(const multiliczba_10& lewo, const multiliczba_10& prawo) {
  uint32_t i = multiliczba_10::wstazki - 1;
  do {
    if (lewo.wstazka[i].schowek < prawo.wstazka[i].schowek) return true;
    if (lewo.wstazka[i].schowek > prawo.wstazka[i].schowek) return false;
    if (i==0) return true;
    i--;
  } while (true);
}


using ML = multiliczba_10;

ML potegiML[10];
uint8_t wziete[9];
uint8_t ilosci[10];
uint8_t N;
uint8_t CYFR;

ML powerML(uint8_t a, uint8_t n) {
  ML v(1);
  while (n > 0) {
    v *= a;
    n--;
  }
  return v; 
}

void inicjuj_potegi(int cyfr) {
  for (int i=0; i<10; i++) {
    potegiML[i] = powerML(i, cyfr);
  }
}

constexpr uint8_t MAX_CYFR = 80;

ML potego_mnozniki[MAX_CYFR][10];

void inicjuj_tabelke_mnoznikow(int cyfr) {
  for (uint8_t i = 0; i <= cyfr; i++) {
    potego_mnozniki[0][i] = ML();
  }
  for (uint8_t i = 0; i < 10; i++) {
    potego_mnozniki[1][i] = potegiML[i];
  }

  for (uint8_t j = 2; j <= cyfr; j++) {
    for(uint8_t i = 0; i < 10; i++) {
      potego_mnozniki[j][i] = potego_mnozniki[j-1][i] + potegiML[i];
    }
  }
}

ML wez_tabelko_mnoznik(uint8_t c, uint8_t n) {
  return potego_mnozniki[n][c];
  if (!potego_mnozniki[n][c].jest_zerem()) {
    return potego_mnozniki[n][c];
  }
  potego_mnozniki[n][c] = potego_mnozniki[n/2][c] + potego_mnozniki[n - n/2][c];
  return potego_mnozniki[n][c];
}




void nowa_metoda_2();

int main(int argc, char** argv)
{
  nowa_metoda_2();
  return 0;
}



struct histogram_cyfr {
  uint8_t v[10] = {0};
  uint8_t& operator[](uint8_t x) {
    return v[x];
  }
  string print() {
    string s;
    char buf[10];
    for (uint8_t i=0; i<10; i++) {
      sprintf(buf, "%2.2d ",v[9-i]);
      s+=buf;
    }
    return s;
    
  }
};

using grupa_cyfr = uint8_t[10];

uint8_t cyfr; //liczba ma miec tyle cyfr

histogram_cyfr cyfry_iteracji; // na poziomie d tylko 9..d maja jakiekolwiek znaczenie
//BI suma_na_poziomie[10]; // suma z iteracji i ze stalej czesci liczby
                         // wyzszy poziom to ustawia przed zejsciem glebiej
                         // poziom sobie to aktualizuje jak przechodzi na nastepna iteracje
                         // i wymienia te cyfry w liczbie ktore sa pod jego kontrola
histogram_cyfr histogram_liczby_poz[10]; // cyfry ustalone na danym poziomie.
                                     // poziom d ustala je sobie przed wejsciem glebiej
uint8_t cyfr_ustalonych_z_liczby[10]; // skrot optymalizacyjny, na poziom = sum(cyfry_z_liczby)
                                      // poziom ustala to przed zejsciem glebiej
uint8_t cyfr_ustalonych_z_iteracji[10]; // skrot optymalizacyjny = suma wymuszonych z iteracji


bool debug = false;





void clear() {
  histogram_cyfr zero;
  cyfry_iteracji = zero;
  for (uint8_t i=0; i<10; i++) {
    histogram_liczby_poz[i] = zero;
    cyfr_ustalonych_z_liczby[i] = 0;
    cyfr_ustalonych_z_iteracji[i] = 0;
  }
}


ML minimum_akceptowalne;
ML maksimum_akceptowalne;

//uint64_t cnt1 = 0;
//uint64_t cnt2 = 0;

template <bool gwarantowane_bycie_w_zakresie>
void jeszcze_nowsza_glebiej(
  const uint8_t d,              // obecna glebokosc - taka cyfre iterujemy
  uint8_t cyfr_znanych_konkretnie, // to sa te pierwsze cyfry w sumie
  //WYLACZNIK histogram_cyfr wziete_hist,     // aktualizujemy 
  //WYLACZNIK2 uint8_t obiecane_i_spelnione_cyfr, // tyle cyfr juz bylo potrzeba ale spelnilismy
  histogram_cyfr obiecane_hist, // takie musimy znalezc cyfry
  uint8_t obiecane_cyfr,        // ilosc cyfr w tym histogramie
  ML obiecane_suma_ML
)
{
  uint8_t iter_do = cyfr - (cyfr_znanych_konkretnie + obiecane_cyfr);
  uint8_t v = obiecane_hist[d];
  for (/*uint8_t v = 0*/; v <= iter_do; v++) {
    if (debug) cout << "d=" << (int)d << " konkret=" << (int)cyfr_znanych_konkretnie
         << " obiecane=" << (int)obiecane_cyfr << endl;
    //WYLACZNIK if (debug) cout << "wziete     =" << wziete_hist.print() << endl;
    if (debug) cout << "obiecane(" << (int)obiecane_cyfr << ")=" << obiecane_hist.print() << endl;
    uint8_t cyfr_do_ustalenia = iter_do - v;

    ML max_reszta_ML = wez_tabelko_mnoznik(d - 1, cyfr_do_ustalenia);
    max_reszta_ML += obiecane_suma_ML;
    
    if (debug) cout << "baza    =" << obiecane_suma_ML.print() << endl;
    if (debug) cout << "baza+max=" << max_reszta_ML.print() << endl;

    if (gwarantowane_bycie_w_zakresie)// || (minimum_akceptowalne <= obiecane_suma_ML && max_reszta_ML < maksimum_akceptowalne))
    {
      //cnt1++;
    }
    else
    {
      //cnt2++;
    if (max_reszta_ML < minimum_akceptowalne) {
      //nawet w maksie nie osiaga wlasciwej wielkosci; moze nastepna iteracja da rade
      goto koncowka_iteracji;
    }
    if (maksimum_akceptowalne <= obiecane_suma_ML) {
      //nawet najmniejsza przekracza zakres, nic z tego już nie bedzie!
      return;
    }    
    if ((maksimum_akceptowalne <= max_reszta_ML) || (obiecane_suma_ML < minimum_akceptowalne)) {
      //tu jest mozliwe, ze glebsza iteracja da rade...
      if (d == 0) {
        assert(d != 0);
        return;
      }
      if (debug) cout << "wymuszone wglebienie" << endl;
      jeszcze_nowsza_glebiej<false>(
        d - 1, 0,
        //WYLACZNIK wziete_hist,
        //WYLACZNIK2 obiecane_i_spelnione_cyfr,
        obiecane_hist,
        obiecane_cyfr,
        obiecane_suma_ML
      );
      goto koncowka_iteracji;
    }
    }
    {
      uint8_t nast_cyfr_znanych_konkretnie = cyfr_znanych_konkretnie;
      //WYLACZNIK histogram_cyfr nast_wziete_hist = wziete_hist;
      //WYLACZNIK2 uint8_t nast_obiecane_i_spelnione_cyfr = obiecane_i_spelnione_cyfr;
      histogram_cyfr nast_obiecane_hist = obiecane_hist;
      uint8_t nast_obiecane_cyfr = obiecane_cyfr;
      ML nast_suma_ML = obiecane_suma_ML;
      
      uint8_t pos = cyfr_znanych_konkretnie;
      for (; pos < cyfr; pos++) {
        uint8_t cyfra_bazy = obiecane_suma_ML.cyfra(cyfr - 1 - pos);
        uint8_t cyfra_maxu = max_reszta_ML.cyfra(cyfr - 1 - pos);
        if (cyfra_bazy != cyfra_maxu) break;
        uint8_t c = cyfra_bazy;
        if (c >= d) {
          // takie cyfry juz ustalilismy
          if (nast_obiecane_hist[c] == 0) {
            // nie ma i nie bedzie, nastepna iteracja
            if (debug) cout << "kibel, nie ma cyfry z iteracji: " << (int)c << endl;
            goto koncowka_iteracji;
          }
          //kasujemy troche dlugu
          nast_obiecane_hist[c]--;
          nast_obiecane_cyfr--;
        } else {
          // nad tym nie pracowalismy, wiec po prostu deklarujemy wziecie
          //WYLACZNIK nast_wziete_hist[c]++;
          nast_suma_ML += potegiML[c];
        }
        nast_cyfr_znanych_konkretnie++;
      }
      if (nast_cyfr_znanych_konkretnie + nast_obiecane_cyfr > cyfr) {
        // nic z tego nie bedzie
        if (debug) cout << "kibel, za duzo znanych cyfr: konkretnie=" << (int)nast_cyfr_znanych_konkretnie
             << " obiecane=" << (int)nast_obiecane_cyfr << endl;
        goto koncowka_iteracji;
      }
      if (nast_cyfr_znanych_konkretnie == cyfr && nast_obiecane_cyfr == 0) {
        if (debug) cout << "sukces ";
        cout << obiecane_suma_ML.print() << endl;
        goto koncowka_iteracji;
      }
      assert(d > 0);
      jeszcze_nowsza_glebiej<true>(
        d - 1,
        nast_cyfr_znanych_konkretnie,
        //WYLACZNIK nast_wziete_hist,
        //WYLACZNIK2 nast_obiecane_i_spelnione_cyfr,
        nast_obiecane_hist,
        nast_obiecane_cyfr,
        nast_suma_ML
      );
    }
  koncowka_iteracji:
    // tego nie trzeba zrobic, jak wychodzimy
    if (v == iter_do) break;
    //WYLACZNIK wziete_hist[d]++;
    obiecane_hist[d]++;
    obiecane_cyfr++;
    obiecane_suma_ML += potegiML[d];
  }
}

void nowa_metoda_2()
{
  //for (int x=0; x<100;x++)
  for (uint8_t CYFR=1; CYFR<=60;CYFR++)
  {
    multiliczba_10::ustaw_wstazke(CYFR / 10 + 1);
    //WYLACZNIK histogram_cyfr wziete_hist;
    histogram_cyfr obiecane_hist;
    cyfr=CYFR;
    inicjuj_potegi(cyfr);
    inicjuj_tabelke_mnoznikow(cyfr);
    
    cout << "szukamy " << (int) cyfr << endl;
    clear();
    minimum_akceptowalne = powerML(10, cyfr-1);
    maksimum_akceptowalne = powerML(10, cyfr);
    jeszcze_nowsza_glebiej<false>(
      9, 0,
      //WYLACZNIK wziete_hist,
      //WYLACZNIK2 0,
      obiecane_hist,
      0, liczba_10(0));
  }
  //cout << "cnt1=" << cnt1 << " cnt2=" << cnt2 << endl;
}




/*
00000000
32/8 = 4

255/10
25*10 = 250
11111010

00010101 = 25
100 -> 128
13
a+b -> ab, carry


   0000000000000000000000000000000

B   01001 01001 01000 00010 +6     2
B+6 01111 01111 01110 01000        8
A   00010 01001 00000 01001        9
+   10100 11000 01110 10001
    -10-6 -10-6    -6 -10-6
       +1          +1              
    1 -> 10000
    0 -> 00110
------------------
         00110
         a0aa0
-----------------
         00110
         10000
1 -> +16
0 -> +6

64/5 = 12.x
 9 9 9 8 2
B    01001 01001 01001 01000 00010 +31-9 = 22 = 10110
B+22 11111 11111 11111 11110 11000
Carry    0     1     0     1     0
   1 00000 00000 11111 11111 11000
no carry - odjac 22
carry - odjac 6
carry 
niemozliwe:
00000 .. 10011
mozliwe:
00000 C
10110 0
10111 1
11000 2
11001 3
11010 4
11011 5
11100 6
11101 7
11110 8
11111 9
0xxxx
~
1xxxx
&
10000
>>, |
>>, |
10011

00000 -> 10100
     10100 10100 11111 11111 11000 -22
 0111 = 7
 1000 = 8
 1001 = 9
 1001 = 9
10010 = 18
10001 = 17
10000 = 16
01111 = 15
      
1xxx->0xxx bylo przeniesienie

a + 6

  0000        nie korektowac bo to 10
  0001        nie korektowac bo to 11
  0010        nie korektowac bo to 12
  0011        nie korektowac bo to 13
  0100        nie korektowac bo to 14
  0101        nie korektowac bo to 15
  0110 = 0    korektowac? X
  0111 = 1    korektowac? X
  1000 = 2    korektowac? X
  1001 = 3    korektowac? X
  1010 = 4    korektowac bezwarunkowo
  1011 = 5    korektowac bezwarunkowo
  1100 = 6    korektowac bezwarunkowo
  1101 = 7    korektowac bezwarunkowo
  1110 = 8    korektowac bezwarunkowo
  1111 = 9    korektowac bezwarunkowo
1 0000 = 10   bylo przeniesienie, nie korektowac
1 0001 = 11   bylo przeniesienie, nie korektowac
1 0010 = 12   bylo przeniesienie, nie korektowac
1 0011 = 13   bylo przeniesienie, nie korektowac
1 0100 = 14   bylo przeniesienie, nie korektowac
1 0101 = 15   bylo przeniesienie, nie korektowac
1 0110 = 16   korektowac? X
1 0111 = 17   korektowac? X
1 1000 = 18   korektowac? X
1 1001 = 19   korektowac? X
1 1010 = impossible

jak nie ma przeniesiania to -6
jak jest przeniesienie to zostawić
jak przetestować przeniesienie?
a>=8 i b>=8 to na pewno przeniesienie
a>=9 i b>=7
a<=3 i b<=3 to na pewno nie ma przeniesienia = warunek X



  0000        nie korektowac bo to 10
  0001        nie korektowac bo to 11
  0010        nie korektowac bo to 12
  0011        nie korektowac bo to 13
  0100        nie korektowac bo to 14
  0101        nie korektowac bo to 15
  0110 = 0    korektowac? X
  0111 = 1    korektowac? X
  1000 = 2    korektowac? X
  1001 = 3    korektowac? X
  1010 = 4    korektowac bezwarunkowo
  1011 = 5    korektowac bezwarunkowo
  1100 = 6    korektowac bezwarunkowo
  1101 = 7    korektowac bezwarunkowo
  1110 = 8    korektowac bezwarunkowo
  1111 = 9    korektowac bezwarunkowo

  000   0
  001   0
  010   0
  011x  x
  100x  x
  101   1
  110   1
  111   1

abcd
czy mamy kolizje:
(a!=b && b==c) => v 
najwyzszy bit:
o=a
akcja w przypadku kolizji:
x


ovx popraweczka  (v&x)|(!v&o)
000 0
001 0
010 0 
011 1
100 1
101 1
110 0
111 1


(o!=v && v==x) => ~o

jak policzyc x (determinant akcji przy kolizyjej wartosci sumy)
P = ab..
Q = cd..
R=P|Q = (a|c)(b|d)..
S=R<<1|R (a|b|c|d)...
S=0... <=> a<=3 && b<=3
  1... w pozostalych
wynik (!x)...

jak policzyc v (czy jest kolizja wartosci)
A = abcd
B=A<<1=bc..
C=A^B=(a!=b)(b!=c)..
D=C^0100=(a!=b)(b==c)..
D=~C=(a==b)(b==c)..
E=C>>1=.(a!=b)(b!=c).
E&D=.((a!=b)&(b==c))..
wynik .v..




  0000 0
  0001 1
  0010 2
  0011 3
  0100 4
  0101 5
  0110 6
  0111 7
  1000 8
  1001 9
  1010 10
  1011 11
  1100 12
  1101 13
  1110 14
  1111 15
1 0000 16
1 0001 17
1 0010 18
1 0011 19

a+3, b+3

a+3<8 i b+3<8 to nie ma przeniesienia
a<5 i b<5 to nie ma przeniesiania


 0 0 9 9 2

20x"9" 1x"8" , 1x"7" 1x"3" 1x"6"
9793600141600921249573819751516253676476425664643L = 9^50*20 + 8^50*1

     53953951279422364398608410217089480477537470L = 7^50*29
>>> 7**50*30

c(50-20-1-1-1-1, 7)



01234567890123456789012345678901234567890123456789

>>> 9**50*20
10307550414640226620729222595312425454042150440020L
>>> 9**50*20 + 8**50*1
10308977662332932580610280881281874949178533186644L
      52155486236774952251988129876519831128286221L
>>> 7**50*29

>>> 9**50*20 + 8**50*2
10310404910025638540491339167251324444314915933268L
      50357021194127540105367849535950181779034972L
>>> 7**50*28

>>> 9**50*20 + 8**50*3
10311832157718344500372397453220773939451298679892L
      48558556151480127958747569195380532429783723L
>>> 7**50*27


>>> 9**50*20 + 8**50*3 + 7**50+1
10311833956183387147784544073501114509100647931142L
         21015313214083865576721629611869943627776L
>>> 6**50*26


>>> 9**50*20 + 8**50*3 + 7**50+1 + 6**50*0
10311833956183387147784544073501114509100647931142L
             2309263891220325604081153869628906250L
>>> 5**50*26
                  32958915605933964438914283339776L
>>> 4**50*26


>>> 9**50*20 + 8**50*10
10321822891567286219539805455006920405405977906260L
      35969300852948242932405606811392986985024980L
>>> 7**50*20
         16165625549295281212862792009130725867520L
>>> 6**50*20


>>> 9**50*20 + 8**50*20
10336095368494345818350388314701415356769805372500L
      17984650426474121466202803405696493492512490L
>>> 7**50*10
          8082812774647640606431396004565362933760L
>>> 6**50*10


>>> 9**50*20 + 8**50*30
10350367845421405417160971174395910308133632838740L


>>> 9**40*7+8**40*0
1034661805900421463212582471444683083207L
    210104590109997923529467359594392033L
>>> 7**40*33
              39894552047282762765303808L
>>> 4**40*33


>>> 9**40*7+8**40*1 7**40*0 6**40*0 
1035991033896206379085486278504963427783L


>>> 9**40*7+8**40*1+7**40*0+6**40*0+5**40*1
1035991033905301326103215560884113818408L
              36267774588438875241185280L
>>> 4**40*30



>>> 9**40*7+8**40*1+7**40*0+6**40*0+5**40*2
1035991033914|396273120944843263264209033L
             | 35058848768824246066479104L
>>> 4**40*29
4*"9", 1*"8", 0*"7" 0*"6" 1*"5"
1*"4", 3*"3", 0*"2" 3*"1" 2*"0"
+ 25 * "43210"



>>> 9**40*7+8**40*1+7**40*2+6**40*2+5**40*2
10360037942609|07168864384461811234610187L
              |31432071309980358542360576L
>>> 4**40*26
9876543210
5110212114
+22 * "43210"

>>> 9**40*7+8**40*1+7**40*2+6**40*2+5**40*2+4**40*1
103600379426090837|7790204076440409316363L
                  | 303941636476423220025L
>>> 3**40*25
9876543210
5000203115
+23 * "3210"

-------------------
>>> 9**40*7+8**40*3+7**40*3+6**40*3+5**40*3+4**40*3
1038668630434831022|277100011472455009069L
                   |218837978263024718418L
>>> 3**40*18
9876543210
0303024223 mam =19
7333330000 musze
7030310000 manko = 14
                  33
7*"3210"

+15 * "3210"
>>> 3**40*11+2**40*2+1**40*2
                    133734322248649472365L



>>> 9**40*7+8**40*4+7**40*4+6**40*3+5**40*3+4**40*3 = 23 => 6
1040004225236376847|177989559967874577646L
                   |194522647344910860816L
>>> 3**40*16
                     72945992754341572806L
>>> 3**40*6
9876543210
0122132314 mam = 19
7443330000 musze
7321200000 manko =15
                  34 zaklepane
                  6 wolne "3210"



>>> 9**40*7+8**40*5+7**40*5+6**40*2+5**40*2+4**40*3
1041339806661333186|217415758245297779022L
                   |194522647344910860816L
>>> 3**40*16
9876543210
1204015042 mam = 19
7552230000 musze
635


>>> 9**40*7+8**40*6+7**40*5+6**40*2+5**40*2+4**40*2
1042669034657116893|164499950676403417422L
                   |194522647344910860816L
>>> 3**40*16
9876543210
2114

>>> 9**40*7+8**40*6+7**40*3+6**40*4+5**40*2+4**40*2
1042656327780584152|795996603483816922572L
                   |194522647344910860816L
>>> 3**40*16
9876543210
0222321322 mam =19
7634220000 musze 
7412


>>> 9**40*7+8**40*6+7**40*3+6**40*4+5**40*3+4**40*1
104265632778967789|0887906271233792607021L
                  | 194522647344910860816L
>>> 3**40*16

9876543210
2243111211 mam = 18
763

*/


