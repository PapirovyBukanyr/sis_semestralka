# Semestrální práce do předmětu principy inteligentních systémů

Tento repozitář obsahuje semestrální práci do předmětu Principy inteligentních systémů na Vysokém učení technickém v Brně. Cílem projektu je vytvořit systém pro sběr, zpracování a analýzu síťových logů pomocí různých modulů a technik umělé inteligence. 

Myšlenka je vytvořit robustní a efektivní nástroj, který umožní monitorování a analýzu síťového provozu v reálném čase, s možností rozšíření o další funkce a moduly. 

## Cíle projektu
- funkční program pro sběr a zpracování síťových logů,
- modularita - systém bude navržen tak, aby umožňoval snadné přidávání nových modulů a funkcí v budoucnu
- efektivita - budu se snažit o optimalizaci výkonu a minimalizaci zátěže systému, což je klíčové pro reálné nasazení. Má představa je, že to bude implementováno na nějakém mikroprocesoru nebo embedded zařízení, tedy se silně omezenými zdroji
- postupné učení se z nových dat - za posledních deset let se chtě nechtě svět internetu proměnil k nepoznání, a tak by bylo vhodné, aby systém dokázal adaptovat své modely na nové vzory a trendy v síťovém provozu
- uživatelské rozhraní - jednoduché a intuitivní uživatelské rozhraní pro snadnou konfiguraci a správu systému

## Struktura repozitáře
- `receiver/` - modul pro příjem a zpracování síťových logů
  - `module1/` - základní modul pro příjem logů
  - `module2/` - modul pro předzpracování dat
  - `module3/` - modul pro analýzu nadcházejících dat pomocí jednoduchých algoritmů
  - `module4/` - modul pro vizualizaci dat
- `sender/` - program pro odesílání síťových logů
- `data/` - data, se kterými bylo pracováno
- `bin/` - složka s .exe soubory připravenými ke spuštění
- `tools/` - složka s užitečnými nástroji, aktuálně se tam nachází nástroj na generování dokumentace

## Jak to vypadá při provozu

Při provozu to samo o sobě nevypadá nijak zvláště zajímavě, neboť byl kladen důraz na efektivitu a nízkou zátěž systému. Proto mi bylo proti srsti implementovat i propojení i s LLM, neboť to zbytečně vyžaduje síťovou komunikaci navíc. Také to buď přestane být lokální řešení (citlivá data se budou toulat světem) nebo to bude vyžadovat další hardware (lokální LLM). Nicméně pro demonstrační účely jsem přidal jednoduchou integraci s OpenAI API, která je implementována v modulu `receiver/module3`. Při spuštění programu se v konzoli vypisují logy, které přicházejí ze senderu, následně jsou předzpracovány a nakonec je odeslán dotaz na OpenAI API, které vrátí odpověď. Ta je označena prefixem `[LLM]`. 

Defaultně je vypnutá ze dvou důvodů. První je vznešený, neb chci se maximálně přiblížit původnímu záměru. Sekundární je zabezpečení proti nechtěnému vyčerpání kreditů na OpenAI API. Neboť v softwaru mám zkompilované exe soubory, za pomoci aplikace proximan by někdo snadno mohl přijít k API klíči. Do budoucna by to chtělo vymyslet jak to ošetřit lépe.

![How it works](data/image.png)


## Využití neuronové sítě v praxi
Síť se průběžně učí z nových dat a je navržena pro zařízení s omezenými zdroji. Implementoval jsem ji jako svou knihovnu pro ESP8266. Na základě routerových logů dokáže předpovídat blížící se problémy s připojením s dostatečným předstihem.
Níže je fotografie použitého zařízení:

![ESP8266 device](data/esp_device.jpg)

