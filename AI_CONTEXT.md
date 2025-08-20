# Context de Setup Proiect Agentie_de_Voiaj

## Configurație IDE și Build System
- **IDE**: Visual Studio 2022
- **Soluție**: Agentie_de_Voiaj
- **Build și Run**: Doar în Visual Studio (nu din command line)
- **Target Platform**: Windows 11
- **Limbaj Principal**: C++

## Structura Proiectelor

### 1. Agentie_de_Voiaj_Server
- **Tip**: C++ Console Application
- **Configurație**: Debug mode
- **Framework**: Qt (Console App cu QTcpServer/QTcpSocket)
- **Responsabilități**: Server TCP pentru comunicația cu clienții

### 2. Agentie_de_Voiaj_Client
- **Tip**: Qt Widgets Application
- **Configurație**: Debug mode
- **Framework**: Qt Widgets
- **Responsabilități**: Interfață grafică pentru utilizatori

## Configurație Dezvoltare
- **Startup Projects**: Server primul, apoi Client
- **Organizare**: Folosim foldere reale, nu filtre VS
- **Qt Modules**: Network, Widgets, și alte module necesare
- **Repository**: Git/GitHub repo inițializat

## Tooling și Workflow
- **Cursor + Claude**: Pentru scrierea codului, refactorizare, documentație
- **Testing UI**: Plan să folosim Squish pentru testarea interfețelor
- **Version Control**: Git cu repository pe GitHub

## Arhitectura Comunicației
- **Protocol**: TCP Sockets
- **Server**: QTcpServer pentru acceptarea conexiunilor
- **Client**: QTcpSocket pentru comunicația cu serverul
- **Comunicație**: Bidirectional între server și client(i)