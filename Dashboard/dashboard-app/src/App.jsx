import React, { useEffect, useState } from "react";
import { ref, onValue, update } from "firebase/database";
import { database } from "./firebase";
import "./dashboard.css";

import { Line } from "react-chartjs-2";

import {
  Chart as ChartJS,
  LineElement,
  CategoryScale,
  LinearScale,
  PointElement,
  Legend,
  Tooltip
} from "chart.js";

ChartJS.register(
  LineElement,
  CategoryScale,
  LinearScale,
  PointElement,
  Legend,
  Tooltip
);

function App() {

  const [page, setPage] = useState("vest");

  const [data, setData] = useState({
  gasLevel: 0,
  airQuality: 0,
  temperature: 0,
  humidity: 0,
  fall: false,
  gasStatus: "",
  airStatus: ""
});

  const [restrictedAlert, setRestrictedAlert] = useState(null);

  const [selectedFallWorker, setSelectedFallWorker] = useState(null);
const [fallHistory, setFallHistory] = useState({});
  const [vests, setVests] = useState([]);
  const [selectedVest, setSelectedVest] = useState("");

  const [workersData, setWorkersData] = useState({});
  const [selectedWorkerModal, setSelectedWorkerModal] = useState(null);

  const [modalZones, setModalZones] = useState({
    Zone1:false,
    Zone2:false,
    Zone3:false
  });

  const [zones, setZones] = useState({
    Zone1:[],
    Zone2:[],
    Zone3:[]
  });

  const [fallLogs, setFallLogs] = useState([]);
  const [zoneLogs, setZoneLogs] = useState([]);

  const [tempGraph, setTempGraph] = useState([]);
  const [humidityGraph, setHumidityGraph] = useState([]);
  const [gasGraph, setGasGraph] = useState([]);
  const [airGraph, setAirGraph] = useState([]);

  /* ================= TIME FORMAT ================= */

  const formatTime = (timestamp) => {

    if (!timestamp) return "-";

    const date = new Date(timestamp * 1000);

    return date.toLocaleString("en-GB", {
      day: "2-digit",
      month: "short",
      year: "numeric",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    });

  };

  /* ================= FIREBASE SENSOR STREAM ================= */

  useEffect(()=>{

    if(!selectedVest) return;

    const vestRef = ref(database, `smart_vests/${selectedVest}`);

    return onValue(vestRef, snap => {

      if(!snap.exists()) return;

      const v = snap.val();

      setData(v);

      setTempGraph(prev => [...prev.slice(-20), v.temperature || 0]);
      setHumidityGraph(prev => [...prev.slice(-20), v.humidity || 0]);
      setGasGraph(prev => [...prev.slice(-20), v.gasLevel || 0]);
      setAirGraph(prev => [...prev.slice(-20), v.airQuality || 0]);

    });

  },[selectedVest]);

  /* ================= VEST LIST ================= */

  useEffect(()=>{

    const vestRef = ref(database,"smart_vests");

    return onValue(vestRef,snap=>{

      if(!snap.exists()) return;

      const vestIds = Object.keys(snap.val());

      setVests(vestIds);

      if(!selectedVest && vestIds.length>0)
        setSelectedVest(vestIds[0]);

    });

  },[]);

  /* ================= WORKERS ================= */

  useEffect(()=>{

    const workersRef = ref(database,"workers");

    return onValue(workersRef,snap=>{

      if(snap.exists())
        setWorkersData(snap.val());

    });

  },[]);

  /* ================= ZONE MAP ================= */

  useEffect(()=>{

    const zoneRef = ref(database,"zones");

    return onValue(zoneRef,snap=>{

      if(!snap.exists()) return;

      const d = snap.val();

      const updatedZones = {
        Zone1:[],
        Zone2:[],
        Zone3:[]
      };

      ["Zone1","Zone2","Zone3"].forEach(zone=>{

        if(!d[zone]) return;

        Object.keys(d[zone]).forEach(workerId=>{

          const worker = workersData[workerId];

          if(worker?.allowedZones?.[zone]===true)
            updatedZones[zone].push(workerId);

        });

      });

      setZones(updatedZones);

    });

  },[workersData]);

  /* ================= ZONE LOGS ================= */

  useEffect(()=>{

    const logsRef = ref(database,"logs");

    return onValue(logsRef,snap=>{

      if(!snap.exists()) return;

      const data = snap.val();
      const logs = [];

      Object.keys(data).forEach(workerId=>{

        Object.keys(data[workerId]).forEach(logId=>{

          const log = data[workerId][logId];

          logs.push({
  workerId,
  zone: log.zone,
  action: log.action,
  timestamp: log.timestamp
});

// trigger alert if restricted
if(log.action === "RESTRICTED"){
  setRestrictedAlert({
    workerId,
    zone: log.zone,
    time: log.timestamp
  });
}

        });

      });

      logs.sort((a,b)=>b.timestamp-a.timestamp);

      setZoneLogs(logs);

    });

  },[]);

  //fall LOgs

  useEffect(()=>{

  const fallRef = ref(database,"fall_logs");

  return onValue(fallRef,snap=>{

    if(!snap.exists()) return;

    const data = snap.val();
    const summary = {};
    const history = {};

    Object.keys(data).forEach(workerId=>{

      const workerData = data[workerId];
      const falls = workerData.logs || {};

      const fallList = Object.values(falls);

      history[workerId] = fallList;

      let lastFall = fallList.sort(
        (a,b)=> new Date(b.time) - new Date(a.time)
      )[0];

      summary[workerId] = {
        workerId,
        count: workerData.count || fallList.length,
        lastZone: lastFall?.zone || "-",
        lastTime: lastFall?.time || "-"
      };

    });

    setFallLogs(Object.values(summary));
    setFallHistory(history);

  });

},[]);

  /* ================= ACTIVE VESTS ================= */

  const activeVests = vests.filter(
    id => workersData[id]?.vestStatus === true
  );

  /* ================= CHART DATA ================= */

  const labels = tempGraph.map((_,i)=>i);

  const createChart = (label,graph)=>({
    labels,
    datasets:[{
      label,
      data:graph,
      borderWidth:2,
      tension:0.3
    }]
  });

  const tempChart = createChart("Temperature",tempGraph);
  const humidityChart = createChart("Humidity",humidityGraph);
  const gasChart = createChart("Gas Level",gasGraph);
  const airChart = createChart("Air Quality",airGraph);

  /* ================= MODAL ================= */

  const openWorkerModal = (workerId)=>{

    setSelectedWorkerModal(workerId);

    const worker = workersData[workerId];

    setModalZones(worker?.allowedZones || {
      Zone1:false,
      Zone2:false,
      Zone3:false
    });

  };

  const toggleModalZone = (zone)=>{

    if(!selectedWorkerModal) return;

    const updated = {
      ...modalZones,
      [zone]:!modalZones[zone]
    };

    setModalZones(updated);

    update(
      ref(database,`workers/${selectedWorkerModal}/allowedZones`),
      updated
    );

  };

  return(

    <div style={{display:"flex"}}>

      {/* SIDEBAR */}

      <div className="sidebar">

        <h3>Smart Construction</h3>

        <button onClick={()=>setPage("vest")}>📊 Dashboard</button>
        <button onClick={()=>setPage("zones")}>📍 Zone Map</button>
        <button onClick={()=>setPage("workers")}>👷 Workers</button>
        <button onClick={()=>setPage("logs")}>📜 Zone Logs</button>
        <button onClick={()=>setPage("falls")}>⚠️ Fall Logs</button>

      </div>

      {/* MAIN */}

      <div className="main">

        {/* DASHBOARD */}

        {page==="vest" && (

          <>
          <h2 className="mb-4">Vest Dashboard</h2>

          <select
          className="form-select mb-4"
          value={selectedVest}
          onChange={e=>setSelectedVest(e.target.value)}
          >

          {activeVests.map(v=>(
            <option key={v}>{v}</option>
          ))}

          </select>

          <div className="row g-4 mb-4">

          <div className="col-md-3">
          <div className="metric-card">
          <div className="metric-title">Temperature</div>
          <div className="metric-value">{data.temperature}°C</div>
          </div>
          </div>

          <div className="col-md-3">
          <div className="metric-card">
          <div className="metric-title">Humidity</div>
          <div className="metric-value">{data.humidity}%</div>
          </div>
          </div>

          <div className="col-md-3">
          <div className="metric-card">
          <div className="metric-title">Gas Level</div>
          <div className="metric-value">{data.gasLevel}</div>
          </div>
          </div>

          <div className="col-md-3">
          <div className="metric-card">
          <div className="metric-title">Air Quality</div>
          <div className="metric-value">{data.airQuality}</div>
          </div>
          </div>

          {/* Gas Status */}
<div className="col-md-4">
<div className="metric-card">
<div className="metric-title">Gas Status</div>
<div className="metric-value">{data.gasStatus}</div>
</div>
</div>

{/* Air Status */}
<div className="col-md-4">
<div className="metric-card">
<div className="metric-title">Air Quality Status</div>
<div className="metric-value">{data.airStatus}</div>
</div>
</div>

{/* Fall Detection */}
<div className="col-md-4">
<div className="metric-card">
<div className="metric-title">Fall Status</div>
<div className="metric-value">
{data.fall ? "⚠️ FALL DETECTED" : "Normal"}
</div>
</div>
</div>

          </div>

          <div className="row">

          <div className="col-md-6 mb-4">
          <div className="graph-card">
          <h5>Temperature</h5>
          <Line data={tempChart}/>
          </div>
          </div>

          <div className="col-md-6 mb-4">
          <div className="graph-card">
          <h5>Humidity</h5>
          <Line data={humidityChart}/>
          </div>
          </div>

          <div className="col-md-6 mb-4">
          <div className="graph-card">
          <h5>Gas Level</h5>
          <Line data={gasChart}/>
          </div>
          </div>

          <div className="col-md-6 mb-4">
          <div className="graph-card">
          <h5>Air Quality</h5>
          <Line data={airChart}/>
          </div>
          </div>

          </div>

          </>
        )}

        {/* ZONE MAP */}

        {page==="zones" && (

          <>
          <h2 className="mb-4">Zone Map</h2>

          <div className="row g-4">

          {["Zone1","Zone2","Zone3"].map((zone,idx)=>(

            <div className="col-md-4" key={zone}>

            <div className="zone-box">

            <h5>{zone}</h5>

            {zones[zone].map(worker=>(
              <span
              key={worker}
              className={`badge m-1 ${
                idx===0?"bg-primary":
                idx===1?"bg-success":"bg-danger"
              }`}
              >
              {worker}
              </span>
            ))}

            </div>

            </div>

          ))}

          </div>
          </>
        )}

        {/* WORKERS */}

        {page==="workers" && (

          <>
          <h2 className="mb-4">Workers</h2>

          <div className="row g-4">

          {Object.keys(workersData).map(workerId=>{

            const worker = workersData[workerId];

            return(

              <div className="col-md-4" key={workerId}>

              <div
              className="metric-card"
              style={{cursor:"pointer"}}
              onClick={()=>openWorkerModal(workerId)}
              >

              <h5>{worker.name || workerId}</h5>

              <p>
              Vest Status
              <span className={`badge ms-2 ${
                worker.vestStatus?"bg-success":"bg-danger"
              }`}>
              {worker.vestStatus?"Active":"Inactive"}
              </span>
              </p>

              <p>Zone: {worker.zone || "Unknown"}</p>

              </div>

              </div>

            );

          })}

          </div>
          </>
        )}

        {/* ZONE LOGS */}

        {page==="logs" && (

          <>
          <h2 className="mb-4">Zone Logs</h2>

          <div className="metric-card">

          <table className="table">

          <thead>
          <tr>
          <th>Worker</th>
          <th>Zone</th>
          <th>Action</th>
          <th>Date & Time</th>
          </tr>
          </thead>

          <tbody>

          {zoneLogs.map((log,index)=>(

            <tr key={index}>

            <td>{log.workerId}</td>

            <td>{log.zone}</td>

            <td>
            <span className={`badge ${
              log.action==="ENTER"
? "bg-success"
: log.action==="EXIT"
? "bg-secondary"
: "bg-danger"
            }`}>
            {log.action}
            </span>
            </td>

            <td>{formatTime(log.timestamp)}</td>

            </tr>

          ))}

          </tbody>

          </table>

          </div>

          </>
        )}

{/* FALL LOGS */}

{page==="falls" && (

<>
<h2 className="mb-4">Fall Detection Summary</h2>

<div className="metric-card">

<table className="table table-hover">

<thead>
<tr>
<th>Worker</th>
<th>Total Falls</th>
<th>Last Zone</th>
<th>Last Fall Time</th>
</tr>
</thead>

<tbody>

{fallLogs.map((log,index)=>(

<tr
key={index}
style={{cursor:"pointer"}}
onClick={()=>setSelectedFallWorker(log.workerId)}
>

<td>
<strong>{log.workerId}</strong>
</td>

<td>
<span className="badge bg-danger">
{log.count}
</span>
</td>

<td>{log.lastZone}</td>

<td>{log.lastTime}</td>

</tr>

))}

</tbody>

</table>

</div>

</>

)}

{selectedFallWorker && (

<div style={{
  position:"fixed",
  top:0,
  left:0,
  width:"100%",
  height:"100%",
  background:"rgba(0,0,0,0.5)",
  display:"flex",
  justifyContent:"center",
  alignItems:"center"
}}>

<div className="metric-card" style={{width:"500px"}}>

<h4>Fall History - {selectedFallWorker}</h4>

<table className="table">

<thead>
<tr>
<th>Zone</th>
<th>Time</th>
</tr>
</thead>

<tbody>

{fallHistory[selectedFallWorker]?.map((fall,i)=>(
<tr key={i}>
<td>{fall.zone}</td>
<td>{fall.time}</td>
</tr>
))}

</tbody>

</table>

<button
className="btn btn-secondary w-100"
onClick={()=>setSelectedFallWorker(null)}
>
Close
</button>

</div>

</div>

)}

         {selectedWorkerModal && (

        <div style={{
          position:"fixed",
          top:0,
          left:0,
          width:"100%",
          height:"100%",
          background:"rgba(0,0,0,0.5)",
          display:"flex",
          justifyContent:"center",
          alignItems:"center"
        }}>

          <div className="metric-card" style={{width:"400px"}}>

            <h4>Zone Permissions</h4>

            {["Zone1","Zone2","Zone3"].map(zone => (

              <div
                key={zone}
                className="d-flex justify-content-between border p-2 my-2"
              >

                <span>{zone}</span>

                <button
                  className={`btn ${
                    modalZones[zone]?"btn-success":"btn-danger"
                  }`}
                  onClick={()=>toggleModalZone(zone)}
                >
                  {modalZones[zone]?"Allowed":"Blocked"}
                </button>

              </div>

            ))}

            <button
              className="btn btn-secondary w-100 mt-3"
              onClick={()=>setSelectedWorkerModal(null)}
            >
              Close
            </button>

          </div>

        </div>

      )}

      {restrictedAlert && (

<div style={{
  position:"fixed",
  top:0,
  left:0,
  width:"100%",
  height:"100%",
  background:"rgba(0,0,0,0.6)",
  display:"flex",
  justifyContent:"center",
  alignItems:"center",
  zIndex:9999
}}>

<div className="metric-card" style={{width:"420px"}}>

<h4 style={{color:"red"}}>🚨 Restricted Access Detected</h4>

<p><b>Worker:</b> {restrictedAlert.workerId}</p>
<p><b>Zone:</b> {restrictedAlert.zone}</p>
<p><b>Time:</b> {formatTime(restrictedAlert.time)}</p>

<button
className="btn btn-danger w-100"
onClick={()=>setRestrictedAlert(null)}
>
Acknowledge
</button>

</div>

</div>

)}

      

      </div>


    </div>

  );

}

export default App;
