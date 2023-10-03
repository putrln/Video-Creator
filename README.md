# README

## Wprowadzenie

Ten program jest używany do pobierania napisów z filmów YouTube, tłumaczenia ich na język polski, generowania opisów obrazów i tworzenia plików audio, a następnie tworzenia pliku video z tych elementów.

## Wymagania

- C++17 lub nowszy
- Biblioteka ffmpeg zainstalowana na systemie

## Sposób użycia

1. Skompiluj program używając kompilatora obsługującego C++17 lub nowszy.

2. Uruchom program. Zostaniesz poproszony o wprowadzenie następujących danych:

   - `Please enter GPT API Key:`: wprowadź swój klucz API dla OpenAI GPT.
   - `Please enter Eleven Labs API Key:`: wprowadź swój klucz API dla Eleven Labs.
   - `Please enter YouTube URL:`: wprowadź adres URL filmu YouTube, z którego chcesz pobrać napisy.

3. Po wprowadzeniu tych danych, program będzie działał automatycznie, wykonując następujące czynności:

   - pobieranie napisów z filmu YouTube
   - tłumaczenie napisów na język polski
   - generowanie propozycji opisów dla czterech obrazów
   - generowanie obrazów na podstawie opisów
   - tworzenie pliku audio z przetłumaczonym tekstem
   - łączenie obrazów i audio w jeden plik wideo

4. Gdy program zakończy działanie, powinieneś zobaczyć plik `result.mkv` w katalogu, z którego uruchomiono program. Ten plik jest wynikiem pracy programu.

## Uwaga

Wszystkie klucze API są poufne i nie powinny być udostępniane osobom trzecim. Zawsze zachowaj ostrożność, przechowując i korzystając z kluczy API.

Przy użyciu tego programu należy przestrzegać zasad korzystania z API OpenAI, Eleven Labs oraz zasady YouTube dotyczące pobierania napisów z filmów.
