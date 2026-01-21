const express = require("express");
const app = express();
const port = 999;

const http = require("http");
const server = http.createServer(app); // express 앱을 http 서버에 올림

// 정적 파일 제공
const path = require("path");
app.use(express.static(__dirname + "/src"));
app.use("/assets", express.static(path.join(__dirname, "public/assets")));

// 기본 라우트
app.get("/", (req, res) => {
  res.sendFile(__dirname + "/src/pages/room.html");
});



let room = {};
/*
room = { // room 예시
  ID : {
    player_name : {green: , red: },
    password : Number,
    setting_time : String, //값 자체는 숫자
    game_player : {green: socket, red: socket},
    game_log: [[[],[]],[[],[]]], //....
    game_turn_num: Number,
    game_stay: {green:0, red:0},
  }

  ID2 : {} // ... ID에 따라 각각의 방 정보 관리
  ID3 : {}
}
*/

// JSON 요청 파싱 설정
app.use(express.json()); // req.body를 사용할 수 있게 함

// POST 요청 처리 예시
app.post("/", (req, res) => {
  const data = req.body; // 클라이언트가 보낸 JSON 데이터

  //console.log("클라이언트 데이터:", data);

  if (data.host === true) {
    // 호스트 정보

    if (room[data.ID] !== undefined) {
      res.json({ type: "ID_error" });
    } else {
      // 우연히 host의 ID가 겹친 경우
      room[data.ID] = {};

      room[data.ID]["player_name"] = {};
      room[data.ID]["password"] = data.password;
      room[data.ID]["setting_time"] = data.time;
      room[data.ID]["game_player"] = {};

      if (data.color === "random") {
        if (Math.random() < 0.5) {
          room[data.ID]["player_name"]["green"] = data.name;
        } else {
          room[data.ID]["player_name"]["red"] = data.name;
        }
      } else if (data.color === "green") {
        room[data.ID]["player_name"]["green"] = data.name;
      } else if (data.color === "red") {
        room[data.ID]["player_name"]["red"] = data.name;
      }
      res.json({ type: "ok" });
    }
  } else if (data.host === false) {
    let ID = Number(data.ID);
    // 게스트 정보
    if (room[ID] !== undefined) {
      if (room[ID]["password"] === data.password) {
        if (room[ID]["player_name"]["green"] === undefined) {
          room[ID]["player_name"]["green"] = data.name;
        } else if (room[ID]["player_name"]["red"] === undefined) {
          room[ID]["player_name"]["red"] = data.name;
        } else {
          room[ID]["player_name"]["extra"] = true;
        }
        res.json({ type: "ok" });
      } else {
        //console.log('비밀번호가 틀렸습니다.');
        res.json({ type: "password_error" });
      }
    } else {
      res.send({ type: "room_error" });
    }
  }
});

app.get("/:room_id", (req, res) => {
  const roomId = req.params.room_id; // 클라이언트가 요청한 방 ID
  //console.log(room);
  //res.send(`방 ${roomId} 접속`);
  if (room[roomId]) {
    res.redirect(`/pages/board.html?roomId=${roomId}`);
  } else {
    //console.log("방이 존재하지 않습니다.");
    res.send({ type: "room_error" });
  }
});

// 서버 시작
server.listen(port, () => {
  console.log(`Example app listening on port ${port}`);
});

// WebSocket 서버 생성
const ws = require("ws");
const webSocketServer = new ws.Server({ server });

// WebSocket 이벤트 처리
webSocketServer.on("connection", (socket, request) => {
  let ID = request.url.slice(1); // 주소에서 방 ID 추출
  ID = Number(ID);

  //let playerId;

  if (room[ID] === undefined || room[ID]["player_name"] === null) {
    // F5 를 통해 재접속 했을 경우.
    return;
  }

  if (Object.keys(room[ID]["player_name"]).length === 1) {
    // host 방생성 후 입장
    let playerId = Object.keys(room[ID]["player_name"])[0];
    room[ID]["game_player"][playerId] = socket;
    let name = room[ID]["player_name"][playerId];

    socket.send(
      JSON.stringify({
        type: "player",
        name: name,
        color: playerId,
        room_id: ID,
      })
    );
  } else if (Object.keys(room[ID]["player_name"]).length === 2) {
    // guest 선착순 입장
    let playerId = Object.keys(room[ID]["player_name"])[1];
    room[ID]["game_player"][playerId] = socket;
    let name = room[ID]["player_name"][playerId];

    socket.send(
      JSON.stringify({
        type: "player",
        name: name,
        color: playerId,
        room_id: ID,
      })
    );

    console.log(room);

    for (let i in room[ID]["game_player"]) {
      room[ID]["game_player"][i].send(
        JSON.stringify({
          type: "time",
          time: room[ID]["setting_time"],
          serverTime: Date.now(),
        })
      );
    }

    for (let i in room[ID]["game_player"]) {
      room[ID]["game_player"][i].send(
        JSON.stringify({ type: "name", name: room[ID]["player_name"] })
      );
    }

    room[ID]["game_player"]["green"].send(
      JSON.stringify({ type: "start", turn: "green" })
    ); // 게임 시작
  } else {
    console.log("방이 꽉 찼습니다.");
    socket.send(JSON.stringify({ type: "full" }));
  }

  room[ID]["game_log"] = [];
  room[ID]["game_turn_num"] = 0;
  room[ID]["game_stay"] = {};

  socket.on("message", (msg) => {
    const data = JSON.parse(msg);
    let ID = Number(data.ID);
    if (data.type === "move") {
      if (socket === room[ID]["game_player"]["green"]) {
        // green

        room[ID]["game_log"][room[ID]["game_turn_num"]] = new Array();
        room[ID]["game_log"][room[ID]["game_turn_num"]][0] = new Array();
        room[ID]["game_log"][room[ID]["game_turn_num"]][1] = new Array();
        room[ID]["game_log"][room[ID]["game_turn_num"]][0].push(
          data.boardState
        );
        room[ID]["game_log"][room[ID]["game_turn_num"]][0].push(
          data.POWState_1
        );
        room[ID]["game_log"][room[ID]["game_turn_num"]][0].push(
          data.POWState_2
        );

        for (let i in room[ID]["game_player"]) {
          room[ID]["game_player"][i].send(
            JSON.stringify({
              type: "move",
              turn: "red",
              boardState: data.boardState,
              POWState_1: data.POWState_1,
              POWState_2: data.POWState_2,
              serverTime: Date.now(),
            })
          );
        }
      } else if (socket === room[ID]["game_player"]["red"]) {
        //red
        room[ID]["game_log"][room[ID]["game_turn_num"]][1].push(
          data.boardState
        );
        room[ID]["game_log"][room[ID]["game_turn_num"]][1].push(
          data.POWState_1
        );
        room[ID]["game_log"][room[ID]["game_turn_num"]][1].push(
          data.POWState_2
        );
        room[ID]["game_turn_num"] += 1;

        for (let i in room[ID]["game_player"]) {
          room[ID]["game_player"][i].send(
            JSON.stringify({
              type: "move",
              turn: "green",
              boardState: data.boardState,
              POWState_1: data.POWState_1,
              POWState_2: data.POWState_2,
              serverTime: Date.now(),
            })
          );
        }
      }
      // 적 진영 도달 조건 red
      if (
        data.boardState["A1"] !== -5 &&
        data.boardState["B1"] !== -5 &&
        data.boardState["C1"] !== -5
      ) {
        room[ID]["game_stay"]["red"] = 0;
        //room[ID]['game_stay']['red'] = 0;
      } else {
        room[ID]["game_stay"]["red"] += 1;
        if (room[ID]["game_stay"]["red"] === 2) {
          for (let i in room[ID]["game_player"]) {
            room[ID]["game_player"][i].send(
              JSON.stringify({
                type: "end",
                win: "red",
                reason: "적 진영 도달",
                game_log: room[ID]["game_log"],
              })
            );
          }
        }
      }
      // 적 진영 도달 조건 green
      if (
        data.boardState["A4"] !== 5 &&
        data.boardState["B4"] !== 5 &&
        data.boardState["C4"] !== 5
      ) {
        room[ID]["game_stay"]["green"] = 0;
      } else {
        room[ID]["game_stay"]["green"] += 1;
        if (room[ID]["game_stay"]["green"] === 2) {
          for (let i in room[ID]["game_player"]) {
            room[ID]["game_player"][i].send(
              JSON.stringify({
                type: "end",
                win: "green",
                reason: "적 진영 도달",
                game_log: room[ID]["game_log"],
              })
            );
          }
        }
      }
    } else if (data.type === "end") {
      if (data.boardState === null) {
        // 시간초과
        for (let i in room[ID]["game_player"]) {
          room[ID]["game_player"][i].send(
            JSON.stringify({
              type: "end",
              win: data.win,
              reason: data.reason,
              game_log: room[ID]["game_log"],
            })
          );
        }
      } else {
        for (let i in room[ID]["game_player"]) {
          if (data.turn === "green") {
            room[ID]["game_log"][room[ID]["game_turn_num"]] = new Array();
            room[ID]["game_log"][room[ID]["game_turn_num"]][0] = new Array();
            room[ID]["game_log"][room[ID]["game_turn_num"]][0].push(
              data.boardState
            );
            room[ID]["game_log"][room[ID]["game_turn_num"]][0].push(
              data.POWState_1
            );
            room[ID]["game_log"][room[ID]["game_turn_num"]][0].push(
              data.POWState_2
            );
          } else if (data.turn === "red") {
            room[ID]["game_log"][room[ID]["game_turn_num"]][1].push(
              data.boardState
            );
            room[ID]["game_log"][room[ID]["game_turn_num"]][1].push(
              data.POWState_1
            );
            room[ID]["game_log"][room[ID]["game_turn_num"]][1].push(
              data.POWState_2
            );
          }
          room[ID]["game_player"][i].send(
            JSON.stringify({
              type: "end",
              reason: data.reason,
              win: data.win,
              boardState: data.boardState,
              POWState_1: data.POWState_1,
              POWState_2: data.POWState_2,
              game_log: room[ID]["game_log"],
            })
          );
        }
      }

      setTimeout(() => {
        delete room[ID];
      }, 1000);
    }
  });

  socket.on("error", (error) => {
    console.log(`에러 발생 : ${error} [${ip}]`);
  });

  socket.on("close", () => {
    // F5, 뒤로가기 등의 접속 이탈 시
    for (let i in room) {
      if (socket === room[i]["game_player"]["red"]) {
        if (room[i]["game_player"]["green"]) {
          room[i]["game_player"]["green"].send(
            JSON.stringify({
              type: "end",
              reason: "상대 퇴장",
              win: "green",
              game_log: room[i]["game_log"],
            })
          );
        } else {
        }
        delete room[i];
      } else if (socket === room[i]["game_player"]["green"]) {
        if (room[i]["game_player"]["red"]) {
          room[i]["game_player"]["red"].send(
            JSON.stringify({
              type: "end",
              reason: "상대 퇴장",
              win: "red",
              game_log: room[i]["game_log"],
            })
          );
        }
        delete room[i];
      }
    }
  });
});
