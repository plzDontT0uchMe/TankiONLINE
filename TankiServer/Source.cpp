#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include<iostream>
#include<locale>
#include<vector>
#include<Windows.h>
#include<fstream>
#include <stdio.h>
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

using namespace std;
using namespace sf;

int playerLastId = 0;
int roomLastId = 0;

class Room
{
public:
	string nameRoom, nameMap;
	int idPlayers[2] = { -1, -1 };
	int idRoom = -1, countPlayers = 0;
	string namePlayers[2] = { "", "" };
	int map[12][16];
};

class Bullet
{
public:
	int id = -1;
	int idPlayer = -1;
	float x = -1, y = -1, angle = -1;
	RectangleShape bullet;
	void move(Vector2f normalizedMovementVec, Time cycleTime)
	{
		x += normalizedMovementVec.x;
		y += normalizedMovementVec.y;

		bullet.setPosition(Vector2f(x - 8, y - 3));

		//cout << x << " | " << y << endl;
	}
};

Bullet bullets[100] = { -1 };
int countBullets = 100;

class Client
{
public:
	int id = -1, roomId = -1, selectMap = -1;
	float angle = -1;
	RectangleShape client;
	unsigned short port;
	string name;
	string nameRoom;
	bool done;
	IpAddress ip;
	Vector2f pos{};
	int hp = 3;
	bool isRegister = false;
	bool isSetRoom = false;
	bool isChangePos = false;
	void move(Vector2f normalizedMovementVec, Time cycleTime)
	{
		pos.x += normalizedMovementVec.x * 0.3;
		pos.y += normalizedMovementVec.y * 0.3;

		client.setPosition(Vector2f(pos.x - 32, pos.y - 32));

		//cout << x << " | " << y << endl;
	}
};

Clock sendRateTimer;
Clock bulletRateTimer;
Clock receiveRateTimer;

Time cycleTime;
Clock cycleTimer;

class Server
{
public:
	int regStep = 0;
	TcpListener listener;
	TcpSocket regSocket;
	Clock sendingRateTimer;
	int sendingRate = 4;
	UdpSocket* dataSocket1;
	Client vectorClients[10];
	Room vectorRooms[10];
	int idBullet = 0;
	IpAddress ip = "127.0.0.1";
	unsigned short port = 8080;
	Thread* thread1;
	Thread* thread2;
	Thread* thread3;
	bool registerTheServer()
	{
		cout << "Запуск сервера..." << endl;
		if (listener.listen(port, ip) == Socket::Status::Done)
		{
			cout << "Сервер успешно запущен" << endl;
			return true;
		}
		else
		{
			cout << "Ошибка запуска сервера" << endl;
			return false;
		}
	}
	void close()
	{
		cout << "Отключение сервера..." << endl;
		listener.close();
		cout << "Сервер отключён" << endl;
	}
	void acceptIncomingConnection()
	{
		listener.accept(regSocket);
		cout << "Клиент подключился" << endl;
	}
	void addClient(Client& c)
	{
		for (int i = 0; i < 10; i++)
		{
			if (vectorClients[i].id == -1)
			{
				vectorClients[i].id = c.id;
				vectorClients[i].name = c.name;
				vectorClients[i].hp = c.hp;
				vectorClients[i].angle = c.angle;
				vectorClients[i].ip = c.ip;
				vectorClients[i].port = c.port;
				vectorClients[i].isChangePos = c.isChangePos;
				vectorClients[i].isRegister = c.isRegister;
				vectorClients[i].roomId = c.roomId;
				break;
			}
		}
	}
	int searchClient(int id)
	{
		for (int i = 0; i < 10; i++)
		{
			if (vectorClients[i].id == id)
			{
				return i;
			}
		}
		return -1;
	}
	void addRoom(int id, int selectMap, string nameRoom)
	{
		for (int i = 0; i < 10; i++)
		{
			if (vectorRooms[i].idRoom == -1)
			{
				vectorRooms[i].idRoom = roomLastId++;
				vectorRooms[i].nameRoom = nameRoom;
				vectorRooms[i].idPlayers[0] = id;
				vectorRooms[i].namePlayers[0] = vectorClients[getIndexById(id)].name;
				cout << vectorRooms[i].namePlayers[0] << endl;
				vectorRooms[i].countPlayers = 1;
				switch (selectMap)
				{
					case 1:
					{
						vectorRooms[i].nameMap = "FOREST";
						ifstream fin("forest.txt");
						if (!fin.is_open())
						{
							cout << "Ошибка открытия файла" << endl;
						}
						else
						{
							while (!fin.eof())
							{
								for (int j = 0; j < 12; j++)
								{
									for (int k = 0; k < 16; k++)
									{
										int block;
										fin >> block;
										vectorRooms[i].map[j][k] = block;
									}
								}
							}
							fin.close();
						}
						break;
					}
				}
				vectorClients[getIndexById(id)].roomId = vectorRooms[i].idRoom;
				break;
			}
		}
	}
	void receiveFromClientRegData()
	{
		Packet receivePacket;
		regSocket.receive(receivePacket);
		if (receivePacket.getDataSize() > 0)
		{
			string name;
			unsigned short TempPort;
			receivePacket >> name;
			receivePacket >> TempPort;
			Client newClient;
			newClient.name = name;
			newClient.ip = regSocket.getRemoteAddress();
			newClient.id = playerLastId++;
			newClient.port = TempPort;
			newClient.isRegister = true;
			newClient.isChangePos = false;
			newClient.isSetRoom = false;
			cout << "NAME - " << newClient.name << endl;
			cout << "IP CLIENT - " << newClient.ip << endl;
			cout << "PORT CLIENT - " << newClient.port << endl;
			addClient(newClient);
		}
	}
	int searchLastClient()
	{
		int max = -1;
		for (int i = 0; i < 10; i++)
		{
			if (vectorClients[i].id > max) max = vectorClients[i].id;
		}
		return getIndexById(max);
	}
	void sendToClientRegData()
	{
		if (vectorClients[searchLastClient()].isRegister)
		{
			vectorClients[searchLastClient()].isRegister = false;
			Packet sendPacket;
			sendPacket << vectorClients[searchLastClient()].id;
			regSocket.send(sendPacket);
		}
	}
	void sendNewClientDataToALL()
	{
		for (int i = 0; i < 10; i++)
		{
			if (vectorClients[i].id == vectorClients[searchLastClient()].id || vectorClients[i].id == -1) continue;
			Packet sendPacket;
			sendPacket << "NEWCLIENT" << vectorClients[searchLastClient()].name;
			cout << vectorClients[i].name << " " << vectorClients[i].port << endl;
			cout << vectorClients[searchLastClient()].name << " " << vectorClients[searchLastClient()].port << endl;
			IpAddress tempIp = vectorClients[i].ip;
			unsigned short tempPort = vectorClients[i].port;
			if (dataSocket1->isBlocking()) dataSocket1->setBlocking(false);
			dataSocket1->send(sendPacket, tempIp, tempPort);
		}
	}
	void registerNewClients()
	{
		while (true)
		{
			acceptIncomingConnection();
			receiveFromClientRegData();
			sendToClientRegData();
			sendNewClientDataToALL();
			sleep(milliseconds(5));
		}
	}
	int getIndexById(int id)
	{
		for (int i = 0; i < 10; i++)
		{
			if (vectorClients[i].id == id) return i;
		}
		return -1;
	}
	int getIndexRoomById(int id)
	{
		for (int i = 0; i < 10; i++)
		{
			if (vectorRooms[i].idRoom == id) return i;
		}
		return -1;
	}
	void receivePacket(Packet& receivedPacket, int index)
	{
		if (index < 0 || index >= 10)
			return;

		IpAddress tempIp = vectorClients[index].ip;
		unsigned short tempPort = vectorClients[index].port;
		if (dataSocket1->isBlocking()) dataSocket1->setBlocking(false);
		dataSocket1->receive(receivedPacket, tempIp, tempPort);
	}
	void sendPacket(Packet& sendedPacket, int index)
	{
		if (index < 0 || index >= 10)
			return;

		IpAddress tempIp = vectorClients[index].ip;
		unsigned short tempPort = vectorClients[index].port;
		if (dataSocket1->isBlocking()) dataSocket1->setBlocking(false);
		dataSocket1->send(sendedPacket, tempIp, tempPort);
	}
	void sendToRoomPacket(Packet& sendedPacket, int indexRoom)
	{
		if (indexRoom < 0 || indexRoom >= 10)
			return;

		int indexPlayer = -1;

		for (int i = 0; i < vectorRooms[indexRoom].countPlayers; i++)
		{
			indexPlayer = getIndexById(vectorRooms[indexRoom].idPlayers[i]);

			if (indexPlayer != -1)
			{
				sendPacket(sendedPacket, indexPlayer);
			}
		}
	}
	int getIndexOtherPlayer(int id, int indexRoom)
	{
		for (int i = 0; i < vectorRooms[indexRoom].countPlayers; i++)
		{
			if (id != vectorRooms[indexRoom].idPlayers[i]) return i;
		}
		return -1;
	}
	int getIdOtherPlayer(int id, int indexRoom)
	{
		for (int i = 0; i < vectorRooms[indexRoom].countPlayers; i++)
		{
			if (id != vectorRooms[indexRoom].idPlayers[i]) return vectorRooms[indexRoom].idPlayers[i];
		}
		return -1;
	}
	void addBullet(Bullet& b)
	{
		for (int i = 0; i < 100; i++)
		{
			if (bullets[i].id == -1)
			{
				bullets[i].id = b.id;
				bullets[i].idPlayer = b.idPlayer;
				bullets[i].x = b.x;
				bullets[i].y = b.y;
				bullets[i].angle = b.angle;
				bullets[i].bullet.setSize(Vector2f(16.0, 6.0));
				break;
			}
		}
	}
	int searchBullet(Bullet b)
	{
		for (int i = 0; i < 100; i++)
		{
			if (bullets[i].id == b.id)
			{
				return i;
			}
		}
		return -1;
	}
	void deleteBullet(Bullet b)
	{
		for (int i = 0; i < 100; i++)
		{
			if (bullets[i].id == b.id)
			{
				bullets[i].id = -1;
				bullets[i].idPlayer = -1;
				bullets[i].x = -1;
				bullets[i].y = -1;
				bullets[i].angle = 0;
				break;
			}
		}
	}
	void receivePacket()
	{
		while (true)
		{
			Packet receivedPacket;
			string namePacket;
			for (int i = 0; i < 10; i++)
			{
				if (vectorClients[i].id == -1) continue;
				receivePacket(receivedPacket, i);
				if (receivedPacket.getDataSize() > 0)
				{
					receivedPacket >> namePacket;
					if (namePacket == "DELETEPLAYER")
					{
						int ID, playersIndex, indexR;
						receivedPacket >> ID;
						playersIndex = getIndexById(ID);
						if (playersIndex != -1)
						{
							indexR = getIndexRoomById(vectorClients[playersIndex].roomId);
							if (indexR != -1)
							{
								Packet s;
								s << "DELETEPLAYER" << vectorClients[playersIndex].id;
								sendToRoomPacket(s, indexR);
								for (int j = 0; j < vectorRooms[indexR].countPlayers; j++)
								{
									if (vectorRooms[indexR].idPlayers[j] == vectorClients[playersIndex].id)
									{
										vectorClients[playersIndex].roomId = -1;
										vectorRooms[indexR].idPlayers[j] = -1;
										vectorRooms[indexR].namePlayers[j] = "";
										vectorRooms[indexR].countPlayers--;
										break;
									}
								}
							}
						}
					}
					if (namePacket == "MESSAGE")
					{
						int index = -1, indexRoom = -1, id;
						string tempName, tempMes;
						receivedPacket >> id;
						receivedPacket >> tempName;
						receivedPacket >> tempMes;
						index = getIndexById(id);
						if (index != -1)
						{
							indexRoom = getIndexRoomById(vectorClients[index].roomId);
							if (indexRoom != -1)
							{
								Packet temp;
								temp << "MESSAGE" << id << tempName << tempMes;
								sendToRoomPacket(temp, indexRoom);
							}
						}
					}
					if (namePacket == "GETROOMS")
					{
						cout << "Получил пакет: " << namePacket << endl;
						Packet send;
						int index = -1, id;
						receivedPacket >> id;
						cout << id << endl;
						index = getIndexById(id);
						if (index != -1)
						{
							send << "GETROOMS";
							for (int j = 0; j < 10; j++)
							{
								if (vectorRooms[j].idRoom == -1) continue;
								send << vectorRooms[j].idRoom << vectorRooms[j].countPlayers;
								for (int k = 0; k < 2; k++)
								{
									if (vectorRooms[j].namePlayers[k] != "") send << vectorRooms[j].namePlayers[k];
								}
							}
							sendPacket(send, index);
						}
					}
					if (namePacket == "CREATEROOM")
					{
						Room tempRoom;
						int index, id, selectMap;
						string nameRoom;
						receivedPacket >> id;
						receivedPacket >> selectMap;
						receivedPacket >> nameRoom;
						index = getIndexById(id);
						addRoom(id, selectMap, nameRoom);
						cout << id << " " << index << " " << vectorClients[index].roomId << endl;
						vectorClients[index].isSetRoom = true;
					}
					if (namePacket == "FINDROOM")
					{
						int index, indexRoom = -1, id, roomId;
						receivedPacket >> id;
						receivedPacket >> roomId;
						index = getIndexById(id);
						if (index != -1)
						{
							indexRoom = getIndexRoomById(roomId);
							if (indexRoom == -1)
							{
								vectorClients[index].roomId = -1;
							}
							else
							{
								if (vectorRooms[indexRoom].countPlayers == 2)
								{
									vectorClients[index].roomId = -1;
								}
								else
								{
									vectorClients[index].roomId = roomId;
									for (int r = 0; r < 2; r++)
									{
										if (vectorRooms[indexRoom].idPlayers[r] == -1)
										{
											vectorRooms[indexRoom].idPlayers[r] = id;
											vectorRooms[indexRoom].namePlayers[r] = vectorClients[index].name;
											vectorRooms[indexRoom].countPlayers++;
										}
									}
								}
							}
							vectorClients[index].isSetRoom = true;
						}
					}
					if (namePacket == "DATA")
					{
						int index, indexRoom, id;
						float posx, posy, angle;
						receivedPacket >> id;
						receivedPacket >> posx;
						receivedPacket >> posy;
						receivedPacket >> angle;
						index = getIndexById(id);
						if (index != -1)
						{
							indexRoom = getIndexRoomById(vectorClients[index].roomId);
							if (indexRoom != -1)
							{
								RectangleShape player(Vector2f(62.0, 62.0));
								vectorClients[index].move(Vector2f(posx, posy), cycleTime);
								player.move(vectorClients[index].pos.x - 32 + 1, vectorClients[index].pos.y - 32 + 1);
								bool isInterect = false;
								for (int k = 0; k < vectorRooms[indexRoom].countPlayers; k++)
								{
									if (id != vectorRooms[indexRoom].idPlayers[k])
									{
										id = vectorRooms[indexRoom].idPlayers[k];
										int ind = getIndexById(id);
										RectangleShape otherPlayer(Vector2f(64.0, 64.0));
										otherPlayer.move(vectorClients[ind].pos.x - 32, vectorClients[ind].pos.y - 32);
										if (otherPlayer.getGlobalBounds().intersects(player.getGlobalBounds()))
										{
											isInterect = true;
										}
										break;
									}
								}
								for (int i = 0; i < 12; i++)
								{
									for (int j = 0; j < 16; j++)
									{
										if (vectorRooms[indexRoom].map[i][j] == 1 || vectorRooms[indexRoom].map[i][j] == 2)
										{
											RectangleShape block(Vector2f(64.0, 64.0));
											block.move(j * 64, i * 64);
											if (block.getGlobalBounds().intersects(player.getGlobalBounds()))
											{
												isInterect = true;
												break;
											}
										}
									}
									if (isInterect) break;
								}
								if (isInterect)
								{
									vectorClients[index].move(Vector2f(-posx, -posy), cycleTime);
								}
								vectorClients[index].angle = angle;
								vectorClients[index].isChangePos = true;
							}
						}
					}
					//Другой игрок другой цвет.
					//Хп при попадании.
					//Кто победил
					if (namePacket == "BULLET")
					{
						int index, id;
						receivedPacket >> id;
						index = getIndexById(id);

						if (index != -1)
						{
							Bullet b;
							b.idPlayer = vectorClients[index].id;
							b.id = idBullet++;
							b.x = vectorClients[index].pos.x;
							b.y = vectorClients[index].pos.y;
							b.bullet.setSize(Vector2f(16.0, 6.0));
							receivedPacket >> b.angle;
							addBullet(b);
						}
					}
				}
			}
		}
	}
	void sendPacket()
	{
		for (int i = 0; i < 10; i++)
		{
			if (vectorClients[i].id == -1) continue;
			if (vectorClients[i].isSetRoom)
			{
				vectorClients[i].isSetRoom = false;
				Packet sendedPacket;
				sendedPacket << "SETROOM" << vectorClients[i].roomId;
				vectorClients[i].hp = 3;
				sendPacket(sendedPacket, i);
				if (vectorClients[i].roomId != -1)
				{
					if (vectorRooms[getIndexRoomById(vectorClients[i].roomId)].countPlayers == 1)
					{
						vectorClients[i].pos.x = 8 * 64 - 32;
						vectorClients[i].pos.y = 10 * 64 - 1;
					}
					if (vectorRooms[getIndexRoomById(vectorClients[i].roomId)].countPlayers == 2)
					{
						vectorClients[i].pos.x = 8 * 64 - 32;
						vectorClients[i].pos.y = 2 * 64 + 1;
					}
					int indexRoom = getIndexRoomById(vectorClients[i].roomId);
					sleep(milliseconds(3));
					sendedPacket.clear();
					sendedPacket << "UPDATEROOM" << vectorRooms[indexRoom].countPlayers;
					for (int j = 0; j < vectorRooms[indexRoom].countPlayers; j++)
					{
						int index = getIndexById(vectorRooms[indexRoom].idPlayers[j]);
						sendedPacket << vectorClients[index].id << vectorClients[index].name << vectorClients[index].pos.x << vectorClients[index].pos.y;
						cout << vectorClients[index].id << " " << vectorClients[index].name << " " << vectorClients[index].pos.x << " " << vectorClients[index].pos.y << endl;
					}
					sendToRoomPacket(sendedPacket, indexRoom);
					indexRoom = getIndexRoomById(vectorClients[i].roomId);
					sleep(milliseconds(3));
					sendedPacket.clear();
					sendedPacket << "SETMAP";
					for (int j = 0; j < 12; j++)
					{
						for (int k = 0; k < 16; k++)
						{
							sendedPacket << vectorRooms[indexRoom].map[j][k];
						}
					}
					sendPacket(sendedPacket, i);
				}
				cout << "Отправил пакет SETROOM" << endl;
			}
			if (vectorClients[i].isChangePos && sendRateTimer.getElapsedTime().asMilliseconds() > 5)
			{
				vectorClients[i].isChangePos = false;
				int indexRoom = getIndexRoomById(vectorClients[i].roomId);
				Packet sendedPacket;
				sendedPacket.clear();
				sendedPacket << "DATA" << vectorRooms[indexRoom].countPlayers;
				//cout << vectorRooms[indexRoom].countPlayers << endl;
				for (int j = 0; j < vectorRooms[indexRoom].countPlayers; j++)
				{
					int index = getIndexById(vectorRooms[indexRoom].idPlayers[j]);
					sendedPacket << vectorClients[index].id << vectorClients[index].pos.x << vectorClients[index].pos.y << vectorClients[index].angle;
					//cout << vectorClients[index].id << " " << vectorClients[index].pos.x << " " << vectorClients[index].pos.y << " " << vectorClients[index].angle << endl;
				}
				sendToRoomPacket(sendedPacket, indexRoom);
				sendRateTimer.restart();
			}
		}

		vector<vector<int>> deleteBullets;

		Packet bulletSync[10];
		bool isBulletSyncSend[10];
		deleteBullets.resize(10);

		for (int r = 0; r < 10; r++)
		{
			if (vectorRooms[r].idRoom == -1) continue;
			bulletSync[r] << "BULLET";
		}

		Packet sendedPacket;

		for (int bc = 0; bc < 100; bc++)
		{
			if (bullets[bc].id == -1) continue;
			sendedPacket.clear();
			int id = bullets[bc].idPlayer;
			int index = getIndexById(id);

			if (index != -1)
			{
				if (bullets[bc].angle == 0.0)
				{
					bullets[bc].move({ 0, -2 }, cycleTime);
				}
				else if (bullets[bc].angle == 90.0)
				{
					bullets[bc].move({ 2, 0 }, cycleTime);
				}
				else if (bullets[bc].angle == 180.0)
				{
					bullets[bc].move({ 0, 2 }, cycleTime);
				}
				else if (bullets[bc].angle == 270.0)
				{
					bullets[bc].move({ -2, 0 }, cycleTime);
				}

				bool isInterect = false;

				int
					deleteRoomIndex = -1,
					indexRoom = getIndexRoomById(vectorClients[index].roomId);

				if (indexRoom == -1)
					continue;

				for (int i = 0, playersIndex = -1; i < vectorRooms[indexRoom].countPlayers; i++)
				{
					playersIndex = getIndexById(vectorRooms[indexRoom].idPlayers[i]);

					if (playersIndex == -1) continue;

					if (vectorClients[index].roomId == vectorClients[playersIndex].roomId)
					{
						bulletSync[indexRoom] << bullets[bc].id << bullets[bc].angle << bullets[bc].x << bullets[bc].y;
						isBulletSyncSend[indexRoom] = true;
					}

					if (playersIndex != -1 && id != vectorClients[playersIndex].id)
					{
						RectangleShape otherPlayer(Vector2f(64.0, 64.0));
						otherPlayer.setPosition(Vector2f(vectorClients[playersIndex].pos.x - 32, vectorClients[playersIndex].pos.y - 32));

						if (otherPlayer.getGlobalBounds().intersects(bullets[bc].bullet.getGlobalBounds()))
						{
							isInterect = true;
							if (--vectorClients[playersIndex].hp == 0)
							{
								sendedPacket.clear();
								sendedPacket << "KILL701" << vectorClients[playersIndex].id;
								sendToRoomPacket(sendedPacket, indexRoom);
								deleteRoomIndex = i;
								vectorClients[playersIndex].roomId = -1;
								break;
							}
							else
							{
								sendedPacket.clear();
								sendedPacket << "SETHEALTH" << vectorClients[playersIndex].id << vectorClients[playersIndex].hp;
								sendToRoomPacket(sendedPacket, indexRoom);
								break;
							}
						}
					}
				}

				if (deleteRoomIndex != -1)
				{
					cout << indexRoom << " " << deleteRoomIndex << " " << vectorRooms[indexRoom].idPlayers[deleteRoomIndex] << endl;
					vectorRooms[indexRoom].idPlayers[deleteRoomIndex] = -1;
					vectorRooms[indexRoom].namePlayers[deleteRoomIndex] = "";
					vectorRooms[indexRoom].countPlayers--;
					deleteRoomIndex = -1;
				}

				if (!isInterect)
				{
					for (int k = 0; k < 12; k++)
					{
						for (int l = 0; l < 16; l++)
						{
							if (vectorRooms[indexRoom].map[k][l] == 1 || vectorRooms[indexRoom].map[k][l] == 2)
							{
								RectangleShape block(Vector2f(64.0, 64.0));
								block.move(l * 64, k * 64);
								if (block.getGlobalBounds().intersects(bullets[bc].bullet.getGlobalBounds()))
								{
									isInterect = true;
									break;
								}
							}
						}
						if (isInterect) break;
					}
				}

				if (isInterect)
				{
					deleteBullets[indexRoom].push_back(bullets[bc].id);
					deleteBullet(bullets[bc]);
				}
			}
		}

		for (int r = 0; r < 10; r++)
		{
			if (vectorRooms[r].idRoom == -1) continue;
			if (isBulletSyncSend[r]) {
				sendToRoomPacket(bulletSync[r], r);
			}

			for (int b = 0; b < deleteBullets[r].size(); b++)
			{
				sendedPacket.clear();
				sendedPacket << "DELETEBULLET" << deleteBullets[r][b];
				sendToRoomPacket(sendedPacket, r);
				deleteBullets[r].erase(deleteBullets[r].cbegin() + b, deleteBullets[r].cbegin() + b + 1);
				b--;
			}
		}
	}
	void work()
	{
		thread1->launch();
		thread2->launch();
	}
	Server()
	{
		dataSocket1 = new UdpSocket;
		dataSocket1->bind(port, ip);
		thread1 = new sf::Thread(&Server::registerNewClients, this);
		thread2 = new sf::Thread(&Server::receivePacket, this);
	}
};

Server server;

void __stdcall TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	server.sendPacket();
}


int main()
{
	setlocale(0, "");
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	for (int i = 0; i < 10; i++)
	{
		server.vectorClients[i].id = -1;
		server.vectorRooms[i].idRoom = -1;
	}

	if (!server.registerTheServer())
	{
		return 0;
	}

	HANDLE ptrTimerHandle;
	CreateTimerQueueTimer(&ptrTimerHandle, NULL, TimerCallback, NULL, 3000, 7, WT_EXECUTEDEFAULT);

	server.work();
	while (true)
	{
		;
	}

	server.close();

	return 1;
}