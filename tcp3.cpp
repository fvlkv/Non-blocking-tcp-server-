#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#include <sys/types.h>
#include <iostream>
#include <istream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
using namespace std; //załączenie odpowiednich bibliotek do korzystania z gniazda Winsock oraz dodatkowych funkcjonalności języka C++ oraz do korzystania z wektorów

int main()
{
	char buf[500]; //utworzenie zmiennej przechowującej dane klienta ograniczenie: 500 znaków
	vector<pollfd> pollVec; //utworzenie wektora do korzystania z struktury pollfd
	WSADATA wsaData; //inicjalizacja biblioteki Winsock po to aby można było utworzyć gniazdo oraz połączenie między klientem a serwerem

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // funckja sprawdzająca czy inicjalizacja biblioteki się powiodła, bez tego nie będziemy w stanie utworzyć gniazda a dalej połączenia
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	char ip4[INET_ADDRSTRLEN];
	struct sockaddr_in myAddr; //utworzenie gniazda: na porcie 2000, korzystanie z ipv4, korzystanie z protokolu TCP, dowolny adrs IP
	int sizemyAddr = sizeof(myAddr);
	myAddr.sin_port = htons(2000);
	myAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "0.0.0.0", &(myAddr.sin_addr));
	memset(myAddr.sin_zero, '\0', sizeof(myAddr.sin_zero));
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) //sprawdzenie czy utworzenie gniazda się powiodło oraz informowanie w konsoli o powodzeniu operacji
		printf("\n error with socket\n");
	else
		printf("\nSocket %d created. \n", sockfd);

	if (bind(sockfd, (struct sockaddr*)&myAddr, sizeof(myAddr)) == -1) { //umożliwienie powiązania się serwera z klientami oraz sprawdzenie czy operacja się powiodła i informacja o tym w konsoli
		perror("bind(divert \n)");
	}
	else {
		printf("bind succesfull \n");
	}

	if ((listen(sockfd, 5)) != 0) { //nasłuchiwanie przez serwer na przychodzące połączenia od klientów oraz sprawdzenie i info. o powodzeniu
		printf("Listen failed\n");
		exit(0);
	}
	else {
		printf("Server Listening... \n");
	}
	sockaddr_in client_addr; //utworzenie gniazda klienta, abyśmy mogli operować na przychodzących klientach
	socklen_t client_addr_size = sizeof(client_addr);
	struct pollfd pollfd;
	pollfd.fd = sockfd; //deklaracja struktury pollfd 
	pollfd.events = POLLRDNORM; //ustawienie początkowej wartości events tak abyśmy mogli operować na otrzymanych danych
	pollfd.revents = 0;  // ustawienie flag  tak aby serwer był gotowy do odbierania danych
	pollVec.push_back(pollfd); //wstawaianie do wektora struktury

	while (1) { //pętla główna w której serwer pracuje cały czas, chyba że wyłączymy program "ręcznie" 
		int ret;
		if (ret = WSAPoll(pollVec.data(), pollVec.size(), 10000) > 0) { // korzystanie z funkcji poll o parametrach: typ danych: wektor, rozmiar danych wektora, czas oczekiwania w ms 10000

			if (pollVec[0].revents & POLLRDNORM && pollVec[0].events & POLLRDNORM) { // umożliwienie otrzymywania danych od klientów bez blokowania

				int fdClient = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size); //akceptowanie przychodzących klientów oraz sprawdzenie czy czynność się powiodła i informowanie o tym w konsoli
				if (fdClient < 0) {
					printf("Connection error");
					continue;
				}
				pollfd.fd = fdClient; //przypisanie gniazda klienta do zmiennej funkcji poll

				pollfd.events |= POLLWRNORM;
				pollVec.push_back(pollfd); //wsadzenie klientów do wektora
			}
			for (int i = 1; i < pollVec.size(); i++) {  //iteroweanie przez przychodzącyh klientów

				if (pollVec[i].revents & POLLRDNORM) { //sprawdzenie  czy przychodzący klient jest w stanie się z nami połączyć za pomocą flagi reevents  

					int y = recv(pollVec[i].fd, buf, sizeof(buf), 0); //otrzymywanie danych od klientów i przypisanie do zmiennej

					if (y < 0) { //jeśli danych nie otrzymano  wyłączenie połączenia i gniazda oraz czyszczenie wektora
						closesocket(pollVec[i].fd);
						pollVec.erase(pollVec.begin() + i);
						break;
					}
					else { //jeśli otrzymano dane przypisywanie ich do zmiennej tekstowej i sprawdzanie poniższych warunków

						string recv; //zmienna do przechowywania danych odebranych od klienta aby móc pomijać znaki niedrukowalne
						for (int j = 0; j < y; j++) { // pętla w której iterujemy poprzez otrzymane dane od klienta a następnie sprawdzamy czy otrzymane znaki nie są znakami niedrukowalnymi

							if (isprint(buf[j])) { //porównanie otrzymanych danych od klientów z niedrukowalnymi znakami a następnie pomijanie ich
								recv += buf[j];
							}

						}

						for (int j = 1; j < pollVec.size(); j++) { //iterowanie przez klientów różnych od klienta od którego otrzymaliśmy dane
							if (j != i) {
								if (pollVec[j].events & POLLWRNORM) { //sprawdzenie czy klienci są gotowi za pomocą flag do otrzymania danych za pomocą funkcji send, jeśli nie pominięcie klienta
									int sending = send(pollVec[j].fd, recv.c_str(), recv.length(), 0); //wysyłanie otrzymanych danych przez klienta do pozostałych klientów, zwracjąc uwagę na to aby rozmiar wysyłanych danych nie zawierał niechcianych znaków zajmujących bufor
									if (sending < 0) { // jeśli wysyłanie danych się nie powiodło informowanie o tym w konsoli i zamknięcie gniazda danego klienta oraz czyszczenie wektora
										printf("Error with sending\n");
										break;
									}

								}

							}
						}

					}
				}
				else if (pollVec[i].revents & POLLHUP) { //jeśli połączenie nieudane, również wyłączenie połączenia i gniazda oraz czyszczenie zmiennych, informuje o tym stan flag reevents
					closesocket(pollVec[i].fd);
					pollVec.erase(pollVec.begin() + i);
					break;
				}

			}

		}
	}
	closesocket(sockfd); //zamknięcie gniazda serwera i wyłączenie biblioteki WSA
	WSACleanup();

}

