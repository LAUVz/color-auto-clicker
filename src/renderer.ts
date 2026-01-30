const { ipcRenderer } = require('electron');

let currentMode: 'target' | 'change' = 'target';
let isDetectingUI = false;
let hasTargetColor = false;

// DOM Elements
const radioTarget = document.getElementById('radioTarget') as HTMLButtonElement;
const radioChange = document.getElementById('radioChange') as HTMLButtonElement;
const pickColorBtn = document.getElementById('pickColorBtn') as HTMLButtonElement;
const toggleBtn = document.getElementById('toggleBtn') as HTMLButtonElement;
const statusBadge = document.getElementById('statusBadge') as HTMLDivElement;
const targetColorBox = document.getElementById('targetColorBox') as HTMLDivElement;
const targetColorLabel = document.getElementById('targetColorLabel') as HTMLDivElement;
const currentColorBox = document.getElementById('currentColorBox') as HTMLDivElement;
const currentColorLabel = document.getElementById('currentColorLabel') as HTMLDivElement;

function colorToHex(color: { r: number; g: number; b: number }): string {
  const r = color.r.toString(16).padStart(2, '0').toUpperCase();
  const g = color.g.toString(16).padStart(2, '0').toUpperCase();
  const b = color.b.toString(16).padStart(2, '0').toUpperCase();
  return `#${r}${g}${b}`;
}

function colorToRgb(color: { r: number; g: number; b: number }): string {
  return `rgb(${color.r}, ${color.g}, ${color.b})`;
}

function updateUI() {
  // Update radio buttons
  radioTarget.classList.toggle('selected', currentMode === 'target');
  radioChange.classList.toggle('selected', currentMode === 'change');
  radioTarget.disabled = isDetectingUI;
  radioChange.disabled = isDetectingUI;

  // Update pick button
  pickColorBtn.disabled = isDetectingUI;

  // Update toggle button
  if (isDetectingUI) {
    toggleBtn.textContent = 'STOP (Numpad 2)';
    toggleBtn.classList.add('stop');
    statusBadge.textContent = 'LIVE';
    statusBadge.classList.add('live');
  } else {
    toggleBtn.textContent = 'START (Numpad 2)';
    toggleBtn.classList.remove('stop');
    statusBadge.textContent = 'OFF';
    statusBadge.classList.remove('live');
  }

  // Enable/disable start button
  const canStart = currentMode === 'change' || hasTargetColor;
  toggleBtn.disabled = !isDetectingUI && !canStart;
}

// Event Listeners
radioTarget.addEventListener('click', () => {
  if (!isDetectingUI) {
    currentMode = 'target';
    ipcRenderer.send('set-mode', currentMode);
    updateUI();
  }
});

radioChange.addEventListener('click', () => {
  if (!isDetectingUI) {
    currentMode = 'change';
    ipcRenderer.send('set-mode', currentMode);
    updateUI();
  }
});

pickColorBtn.addEventListener('click', () => {
  if (!isDetectingUI) {
    ipcRenderer.send('pick-color');
  }
});

toggleBtn.addEventListener('click', () => {
  ipcRenderer.send('toggle-detection');
});

// IPC Listeners
ipcRenderer.on('color-picked', (_, color: { r: number; g: number; b: number }) => {
  hasTargetColor = true;
  targetColorBox.style.background = colorToRgb(color);
  targetColorLabel.textContent = colorToHex(color);
  updateUI();

  // Play beep sound (using Web Audio API)
  const audioContext = new AudioContext();
  const oscillator = audioContext.createOscillator();
  const gainNode = audioContext.createGain();
  oscillator.connect(gainNode);
  gainNode.connect(audioContext.destination);
  oscillator.frequency.value = 1500;
  oscillator.type = 'sine';
  gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
  oscillator.start();
  oscillator.stop(audioContext.currentTime + 0.1);
});

ipcRenderer.on('current-color', (_, color: { r: number; g: number; b: number }) => {
  currentColorBox.style.background = colorToRgb(color);
  currentColorLabel.textContent = colorToHex(color);
});

ipcRenderer.on('detection-started', () => {
  isDetectingUI = true;
  updateUI();

  // Play start beep
  const audioContext = new AudioContext();
  const oscillator = audioContext.createOscillator();
  const gainNode = audioContext.createGain();
  oscillator.connect(gainNode);
  gainNode.connect(audioContext.destination);
  oscillator.frequency.value = 1000;
  oscillator.type = 'sine';
  gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
  oscillator.start();
  oscillator.stop(audioContext.currentTime + 0.1);
});

ipcRenderer.on('detection-stopped', () => {
  isDetectingUI = false;
  updateUI();

  // Play stop beep
  const audioContext = new AudioContext();
  const oscillator = audioContext.createOscillator();
  const gainNode = audioContext.createGain();
  oscillator.connect(gainNode);
  gainNode.connect(audioContext.destination);
  oscillator.frequency.value = 600;
  oscillator.type = 'sine';
  gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
  oscillator.start();
  oscillator.stop(audioContext.currentTime + 0.1);
});

ipcRenderer.on('click-triggered', () => {
  // Play click triggered beep
  const audioContext = new AudioContext();
  const oscillator = audioContext.createOscillator();
  const gainNode = audioContext.createGain();
  oscillator.connect(gainNode);
  gainNode.connect(audioContext.destination);
  oscillator.frequency.value = 2000;
  oscillator.type = 'sine';
  gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
  oscillator.start();
  oscillator.stop(audioContext.currentTime + 0.1);
});

// Initialize
updateUI();
