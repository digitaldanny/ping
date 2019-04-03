/*
 * Game.c
 *
 *  Created on: Apr 2, 2019
 *      Author: dhami
 */

#include "Game.h"

/*
 * DESCRIPTION: Thread for the host to create a game.
 *
 * STEPS:
 * 1. Initialize players
 * 2. Establish connection with client (use LED to indicate WiFi conn.)
 * 3. Try to receive packet from the client. (while)
 * 4. Acknowledge client once the client joins.
 * 5. Initialize the board (draw arena, players, scores)
 * 6. Add GenerateBall, DrawObjects, ReadJoystickHost, SendDataToClient
 *      ReceiveDataFromClient, MoveLEDs (low priority), Idle
 * 7. Kill self.
 */
void CreateGame()
{

}
