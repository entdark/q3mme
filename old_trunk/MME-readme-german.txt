Hallo.

Quake 3 Movie Maker's Edition ist eine spezielle Version der Quake 3 Engine,
die es erleichtern soll, Game Movies zu erstellen. Sie macht es möglich,
Aufgaben einfacher durchzuführen und fügt neue Funktionen zur Engine hinzu,
die sich auf Moviemaking beziehen.

Diese Version - 1.33.4.1.

Unterschiede zu Quake 3:
  * Es gibt die Möglichkeit, die Skybox mit einer beliebigen soliden Farbe zu
    füllen, auch bei 'Space' Maps, ohne etwas an der Map zu verändern.
  * Einer beliebigen Spielernummer kann ein bestimmtes Model zugewiesen
    werden.
  * TGAs können mit RLE Kompression komprimiert werden.
  * Während Avidemo können JPEG Screenshots gemacht werden.
  * Während Avidemo kann WAV sound gespeichert werden.
  * Die Präzision der Avidemo Frame Rate wurde erhöht, jetzt wird genau in der
    gleichen Zahl FPS aufgenommen, wie cl_avidemo spezifiziert wurde. 24FPS
    werden kein 24,39FPS Output und 300FPS kein 333FPS Output mehr erstellen.
    (sehr nützlich im Bezug auf TGA_Hook)
  * Eine Demo wird bei Drücken einer beliebigen Taste nicht mehr gestoppt.
  * Einige Bugs in der Engine wurden behoben.
  * Die Anzahl der möglichen Screenshots ist nicht mehr auf 10000 begrenzt.
  * Fenstermodus schaltet die Hardware Overbright Unterstützung nicht aus, das
    wird helfen, bei jeder beliebigen Auflösung zu rendern, komplett ohne den
    Vollbildmodus zu verwenden.
  * Screenshots werden im Unterverzeichnis mit dem Namen der Demo platziert.


Es funktioniert mit OSP, Defrag, CPMA und jeder anderen Quake 3 Mod, die mit
dem originalen Quake 3 v1.32 gut läuft.

Als zusätzliches Plus läuft es auch mit TGA_Hook: Wähle TGA Screenshot Format
und schalte TGA RLE Kompression aus. Wahrscheinlich musst du den TGA_Hook
exe Dateipfad in der Registry modifizieren. Gehe in der Registry zum Schlüssel
"HKEY_LOCAL_MACHINE\SOFTWARE\14K\Hooker\ExeLocation" und gib statt dem Pfad
der quake3.exe den Pfad deiner quake3mme.exe ein. Das Feature der Screenshot-
Benennung wird wegen offensichtlicher Gründe nicht funktionieren: TGA_Hook
generiert die Namen der Screenshots.

Sei vorsichtig mit dem Export bei Fenstermodus: Du musst immernoch Quake3MME
im Vordergrund lassem, damit du korrekte Screenshots bekommst.

Viele Leute finden es schade, dass das Rendern mit Avidemo nicht Realzeit ist.
Es rendert, bis es fertig ist und die Frame Rate der Ausgabedatei hängt dabei
nicht davon ab, wie gut deine Hardware ist.


Neue Konsolenbefehle in Quake3MME:
  * mme_skykey <Nummer>   - Wenn nicht gleich Null, dann wird Skybox mit einer
                            soliden Farbe gefüllt.
                 (standard) 0 = ausgeschaltet
                            1 = rot
                            2 = grün
                            3 = gelb
                            4 = blau
                            5 = cyan
                            6 = magenta
                            7 = weiss
                            8 = schwarz

  * mme_playernumbers     - Zeigt die einem Spieler zugeordnete Spielernummer.
                            Spielernummern werden für die folgenden Konsolen-
                            Befehle gebraucht.

  * mme_forcemodelplayerX - Force Model für Spieler X. Insgesamt 16 Spielern
                            können bestimmte Models zugewiesen werden. ACHTUNG:
                            Damit dies funktioniert müssen 'cg_enemymodel' und
                            'cg_forcemodel' ausgeschaltet sein.

  * mme_wavdemo_enabled   - Kontrolliert die Aufnahme von .wav Sound während
                            Avidemo. Als Nebeneffekt gibt es keinen Sound mehr
                            durch die Soundkarte. Der Sound wird nach
                            "screenshots/wav" exportiert.
                 (standard) 0 = es wird kein Sound exportiert
                            1 = Sound wird während Screenshooting exportiert.
                            2 = Screenshooting wird übersprungen und es wird
                                nur Sound exportiert

  * mme_screenshot_format - Selbsterklärend - Das Format der Screenshots.
                 (standard) tga  = Screenshots werden in TGA exportiert
                            jpeg = Screenshots werden in JPEG exportiert

  * mme_tga_rle_compress  - TGA RLE Kompression einschalten. Spart ein
                            bisschen Speicherplatz.
                            0 = TGA RLE Kompression ausschalten
                 (standard) 1 = TGA RLE Kompression einschalten

  * mme_jpeg_quality      - Stellt den Level der Kompression für JPEG
                            Screenshots ein, sofern JPEG Screenshots
                            verwendet werden.
                            0-100 = Kompressionslevel
                               90 = standard

  * mme_jpeg_optimize_huffman_tables
                          - Spart ein bisschen Platz auf der Festplatte
                            durch Optimierung der Screenshots, doch
                            dadurch wird das Screenshooting ein wenig
                            verlangsamt.
                            0 = Schaltet "optimized huffman tables" aus
                 (standard) 1 = Schaltet "optimized huffman tables" ein

  * mme_jpeg_downsample_chroma
                          - Schreibt den Farbbestandteil mit niedrigerer
                            Auflösung, spart Platz in Kompromiss mit der
                            Qualität. Sollte ausgeschaltet bleiben, ausser
                            es wird mit sehr hoher Auflösung aufgezeichnet.
                 (standard) 0 = Schaltet 'chroma downsample' aus
                            1 = Schaltet 'chroma downsample' ein

  * mme_anykeystopsdemo   - Verhindert, dass das Drücken beliebiger Tasten
                            das Abspielen der Demo stoppt. Diese Tasten
                            können stattdessen mit Funktionen belegt werden.
                            0 = Tastendruck beendet Abspielen der Demo
                 (standard) 1 = Tastendruck beendet Abspielen der Demo nicht

  * mme_roundmode         - Wenn auf 1 gesetzt, wird die Berechnung der Phsyik
                            genauer ausgeführt, weil der Rundungsprozess wie
                            in C verläuft. Beeinflusst Strafejumping während
                            des Spiels (bei Demo Wiedergabe nicht), also habe
                            ich eine Option eingebaut, die das Umschalten zum
                            alten Q3 Style möglich macht.
                            0 = Berechnung der Spielphysik wird nicht geändert
                 (standard) 1 = Berechnung der Spielphysik wird genauer durch-
                                geführt. Beeinflusst Demo Aufnahme, nicht Demo
                                Wiedergabe.

----------------------------
Kontakt: HMage - hmd@mail.ru

http://hmd.c58.ru/muwiki/quake3mme
http://sourceforge.net/projects/quake3mme

------
Thanks

Ich möchte jrb von shaolinproductions für seine Unterstützung und das
Bearbeiten der englischen Version dieser Datei danken.

Ich möchte Auri für seine Unterstützung und die Bearbeitung der
deutschen Version dieser Datei danken.

----------
Change Log

1.33.4:
  * Probleme beim Verwenden mit TGA_Hook beseitigt.
  * Fenstermodus deaktiviert das Overbright Bits Feature nicht.
  * Screenshots werden basierend auf dem Namen der Demo benannt und in einem
    Unterverzeichnis abgelegt.
  * WAV Sound wird in einem Unterverzeichnis mit dem Namen der Demo abgelegt.
  * Bugfixes von icculus.org/quake3 angefügt.

1.33.3:
 * Export von WAV Sound ermöglicht.
 * Präzision der Avidemo Frame Rate erhöht.
 * Währen Avidemo können JPEG Screenshots gemacht werden.
 * Alpha Buffer Initialisierungscode angefügt, wird aber noch nirgends
   verwendet.
 * Bug "Unknown Client Command" am Anfang einer Map behoben.
 * Englische Readme-Datei angefügt.

1.33.2:
 * Russische Readme-Datei angefügt.
 * Die Skybox kann mit einer soliden Farbe gefüllt werden.
 * Ein beliebiges Model kann einer bestimmten Spielernummer zugeordnet werden.
 * Wiedergabe einer Demo wird nicht durch Drücken beliebiger Tasten gestoppt.
 * Qualität des Rundungsprozesses bei Berechnung der Spielphysik kann mit
   Hilfe eines Konsolenbefehls erhöht werden.

1.33.1:
 * Begrenzung von 10000 Screenshots entfernt.
