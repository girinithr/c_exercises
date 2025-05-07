const board = document.getElementById('board');
const status = document.getElementById('status');


const socket = new WebSocket('ws://localhost:8000');

let myTurn = false;
let symbol = '';

function drawBoard() {
  board.innerHTML = '';
  let cell = 1;
  for (let i = 0; i < 3; i++) {
    const row = document.createElement('tr');
    for (let j = 0; j < 3; j++) {
      const td = document.createElement('td');
      td.dataset.cell = cell++;
      td.addEventListener('click', () => {
        if (myTurn && td.textContent === '') {
          socket.send(td.dataset.cell);
        }
      });
      row.appendChild(td);
    }
    board.appendChild(row);
  }
}

socket.onopen = () => {
  status.textContent = 'Connected. Waiting for opponent...';
};

socket.onmessage = (event) => {
  const msg = event.data;

  if (msg.startsWith('You are')) {
    symbol = msg.split(' ')[2];
    status.textContent = `You are ${symbol}.`;
  } else if (msg.startsWith('Your move')) {
    myTurn = true;
    status.textContent = "Your turn.";
  } else if (msg.startsWith('Opponent move')) {
    myTurn = false;
    status.textContent = "Opponent's turn.";
  } else if (msg.startsWith('Board:')) {
    const cells = msg.slice(6).trim();
    const tds = board.querySelectorAll('td');
    for (let i = 0; i < 9; i++) {
      tds[i].textContent = cells[i] === '.' ? '' : cells[i];
    }
  } else {
    status.textContent = msg;
  }
};

socket.onclose = () => {
  status.textContent = 'Connection closed.';
};

drawBoard();
