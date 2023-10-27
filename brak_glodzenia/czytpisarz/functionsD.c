/**
*@project Projekt 2 - Czytelnicy i Pisarze
*@author Dawid Ugniewski (109522), PS2
*@version 1.1.0
*/

#include <time.h>

int LosujKilkaSekund()
{
    int wylosowana_liczba;
    wylosowana_liczba = ((rand() % 9) + 1); //zakres [1;9]
    return wylosowana_liczba;
}