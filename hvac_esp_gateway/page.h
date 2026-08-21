#ifndef PAGE_H
#define PAGE_H

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HVAC</title>
<style>
  :root{
    --bg:#0e1116; --card:#161b22; --line:#262c36;
    --text:#e6edf3; --dim:#7d8590; --accent:#58a6ff;
  }
  *{box-sizing:border-box}
  body{
    margin:0; min-height:100vh;
    display:flex; align-items:center; justify-content:center;
    background:var(--bg); color:var(--text);
    font-family:-apple-system,Segoe UI,Roboto,sans-serif;
    padding:1.5rem;
  }
  .card{
    width:100%; max-width:400px;
    background:var(--card); border:1px solid var(--line);
    border-radius:12px; padding:1.5rem 1.75rem;
  }
  header{
    display:flex; align-items:baseline; justify-content:space-between;
    padding-bottom:1rem; margin-bottom:.5rem;
    border-bottom:1px solid var(--line);
  }
  h1{font-size:1rem; font-weight:600; margin:0; letter-spacing:.02em}
  .sub{font-size:.7rem; color:var(--dim); text-transform:uppercase; letter-spacing:.1em}

  .row{
    display:flex; align-items:baseline; justify-content:space-between;
    padding:.85rem 0; border-bottom:1px solid var(--line);
  }
  .row:last-of-type{border-bottom:none}
  .lbl{font-size:.7rem; color:var(--dim); text-transform:uppercase; letter-spacing:.12em}
  .val{
    font-family:ui-monospace,SFMono-Regular,Consolas,monospace;
    font-size:1.9rem; font-variant-numeric:tabular-nums; line-height:1;
  }
  .val .u{font-size:.9rem; color:var(--dim); margin-left:.15rem}
  .val.set{font-size:1.2rem; color:var(--dim)}

  .badge{
    font-size:.75rem; font-weight:600; letter-spacing:.1em;
    padding:.3rem .7rem; border-radius:999px;
    border:1px solid var(--line); color:var(--dim);
  }
  .badge.s0{color:#7d8590}
  .badge.s1{color:#58a6ff; border-color:#1f3a5f; background:#0d1f33}
  .badge.s2{color:#ff9d5c; border-color:#5a3620; background:#2b1a0f}
  .badge.s3{color:#a371f7; border-color:#3c2a5c; background:#1c1329}

  #stale{
    margin-top:1rem; font-size:.75rem; color:#f85149;
    min-height:1rem; text-align:center;
  }
  .dot{
    width:7px; height:7px; border-radius:50%;
    background:var(--accent); display:inline-block; margin-right:.4rem;
    animation:pulse 2s ease-in-out infinite;
  }
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.25}}
</style>
</head>
<body>
  <div class="card">
    <header>
      <h1><span class="dot"></span>HVAC</h1>
      <span class="sub">bench system</span>
    </header>

    <div class="row">
      <span class="lbl">Room</span>
      <span class="val"><span id="room">--</span><span class="u">&deg;F</span></span>
    </div>
    <div class="row">
      <span class="lbl">Duct</span>
      <span class="val"><span id="duct">--</span><span class="u">&deg;F</span></span>
    </div>
    <div class="row">
      <span class="lbl">Setpoint</span>
      <span class="val set"><span id="set">--</span><span class="u">&deg;F</span></span>
    </div>
    <div class="row">
      <span class="lbl">Vent</span>
      <span class="badge" id="state">--</span>
    </div>

    <div id="stale"></div>
  </div>

<script>
const NAMES = ["CLOSED","COOLING","HEATING","AWAY"];
const f = v => (v * 0.018 + 32).toFixed(1);

async function tick() {
  try {
    const res = await fetch('/data');
    if (!res.ok) { document.getElementById('stale').textContent = 'waiting for data...'; return; }
    const d = await res.json();
    document.getElementById('room').textContent  = f(d.room);
    document.getElementById('duct').textContent  = f(d.duct);
    document.getElementById('set').textContent   = f(d.set);
    document.getElementById('state').textContent = NAMES[d.state];
    document.getElementById('state').className   = 'badge s' + d.state;
    document.getElementById('stale').textContent = '';
  } catch (e) {
    document.getElementById('stale').textContent = 'no connection';
  }
}
tick();
setInterval(tick, 2000);
</script>
</body></html>
)rawliteral";

#endif
