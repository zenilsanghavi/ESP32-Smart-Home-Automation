import { useCallback, useEffect, useRef, useState } from 'react';
import {
  ActivityIndicator,
  KeyboardAvoidingView,
  Platform,
  Pressable,
  ScrollView,
  StyleSheet,
  Switch,
  Text,
  TextInput,
  View,
} from 'react-native';
import { StatusBar } from 'expo-status-bar';
import { SafeAreaProvider, SafeAreaView } from 'react-native-safe-area-context';
import { getStatus, setRelay } from './src/api';
import { loadSettings, saveSettings } from './src/storage';

const POLL_MS = 4000; // same as the web UI, so voice changes show up
const ICONS = ['🛏', '🛋', '💡', '⚡'];

const C = {
  bg: '#0a0e1a',
  panel: '#0f1628',
  border: '#1e3a5f',
  accent: '#00d4ff',
  on: '#00ff88',
  off: '#ff3366',
  text: '#c8dff5',
  dim: '#4a6080',
};

export default function App() {
  return (
    <SafeAreaProvider>
      <StatusBar style="light" />
      <RelayScreen />
    </SafeAreaProvider>
  );
}

function RelayScreen() {
  const [settings, setSettings] = useState(null);
  const [states, setStates] = useState([false, false, false, false]);
  const [connection, setConnection] = useState('idle'); // idle | ok | error
  const [busy, setBusy] = useState(null); // channel currently being switched
  const [showSettings, setShowSettings] = useState(true);
  const [lastHost, setLastHost] = useState('');
  const busyRef = useRef(false);

  // The ESP32 moves between networks, so its IP is asked for on every launch.
  // The last one used is only kept to prefill the field.
  useEffect(() => {
    loadSettings().then((s) => {
      setLastHost(s.host);
      setSettings({ ...s, host: '' });
    });
  }, []);

  const host = settings?.host;

  const refresh = useCallback(async () => {
    if (!host || busyRef.current) return;
    try {
      setStates(await getStatus(host));
      setConnection('ok');
    } catch {
      setConnection('error');
    }
  }, [host]);

  useEffect(() => {
    if (!host) return;
    refresh();
    const id = setInterval(refresh, POLL_MS);
    return () => clearInterval(id);
  }, [host, refresh]);

  const send = async (channel, on) => {
    if (!host) return;
    busyRef.current = true;
    setBusy(channel);
    try {
      setStates(await setRelay(host, channel, on));
      setConnection('ok');
    } catch {
      setConnection('error');
    } finally {
      busyRef.current = false;
      setBusy(null);
    }
  };

  const updateSettings = (next) => {
    setSettings(next);
    saveSettings(next);
  };

  if (!settings) {
    return (
      <View style={[styles.fill, styles.center]}>
        <ActivityIndicator color={C.accent} />
      </View>
    );
  }

  return (
    <SafeAreaView style={styles.fill}>
      <KeyboardAvoidingView
        style={styles.fill}
        behavior={Platform.OS === 'ios' ? 'padding' : undefined}
      >
        <ScrollView contentContainerStyle={styles.content} keyboardShouldPersistTaps="handled">
          <Header
            host={host}
            connection={connection}
            onSettings={() => setShowSettings((v) => !v)}
          />

          {showSettings && (
            <SettingsPanel
              settings={settings}
              initialHost={host || lastHost}
              onSave={(next, initialStates) => {
                updateSettings(next);
                setStates(initialStates);
                setConnection('ok');
                setShowSettings(false);
              }}
            />
          )}

          {settings.names.map((name, i) => (
            <RelayCard
              key={i}
              index={i}
              name={name}
              on={states[i]}
              busy={busy === i + 1 || busy === 'all'}
              disabled={!host}
              onToggle={(on) => send(i + 1, on)}
            />
          ))}

          <View style={styles.masterRow}>
            <MasterButton label="All On" color={C.on} disabled={!host || busy !== null} onPress={() => send('all', true)} />
            <MasterButton label="All Off" color={C.off} disabled={!host || busy !== null} onPress={() => send('all', false)} />
          </View>

          {connection === 'error' && (
            <Text style={styles.hint}>
              Can't reach the ESP32 at {host}. Make sure your phone is on the same Wi-Fi and
              the IP matches the one printed in the Serial Monitor.
            </Text>
          )}
        </ScrollView>
      </KeyboardAvoidingView>
    </SafeAreaView>
  );
}

function Header({ host, connection, onSettings }) {
  const dot = connection === 'ok' ? C.on : connection === 'error' ? C.off : C.dim;
  const label =
    !host ? 'NOT CONFIGURED' : connection === 'ok' ? `CONNECTED · ${host}` : connection === 'error' ? `OFFLINE · ${host}` : `CONNECTING · ${host}`;
  return (
    <View style={styles.header}>
      <View style={styles.flex1}>
        <Text style={styles.logo}>
          ESP<Text style={{ color: '#ff6b35' }}>32</Text> RELAY
        </Text>
        <View style={styles.statusRow}>
          <View style={[styles.dot, { backgroundColor: dot }]} />
          <Text style={styles.statusText} numberOfLines={1}>{label}</Text>
        </View>
      </View>
      <Pressable onPress={onSettings} hitSlop={12} style={styles.iconBtn} accessibilityLabel="Settings">
        <Text style={styles.iconBtnText}>⚙</Text>
      </Pressable>
    </View>
  );
}

function SettingsPanel({ settings, initialHost, onSave }) {
  const [host, setHost] = useState(initialHost);
  const [names, setNames] = useState(settings.names);
  const [checking, setChecking] = useState(false);
  const [error, setError] = useState('');

  const cleanHost = host.trim().replace(/^https?:\/\//, '').replace(/\/+$/, '');

  const connect = async () => {
    if (!cleanHost) {
      setError('Enter the IP address first.');
      return;
    }
    setChecking(true);
    setError('');
    try {
      const states = await getStatus(cleanHost);
      onSave({ host: cleanHost, names: names.map((n, i) => n.trim() || `Relay ${i + 1}`) }, states);
    } catch {
      setError(`No ESP32 found at ${cleanHost}. Check the IP and that your phone is on the same Wi-Fi.`);
    } finally {
      setChecking(false);
    }
  };

  return (
    <View style={styles.panel}>
      <Text style={styles.panelTitle}>ESP32 IP ADDRESS</Text>
      <TextInput
        value={host}
        onChangeText={setHost}
        placeholder="e.g. 192.168.1.50"
        placeholderTextColor={C.dim}
        keyboardType="numbers-and-punctuation"
        autoCapitalize="none"
        autoCorrect={false}
        returnKeyType="go"
        onSubmitEditing={connect}
        style={styles.input}
      />
      <Text style={styles.panelNote}>Shown in the Serial Monitor after the ESP32 connects to Wi-Fi.</Text>

      <Text style={[styles.panelTitle, { marginTop: 16 }]}>RELAY NAMES</Text>
      {names.map((n, i) => (
        <TextInput
          key={i}
          value={n}
          onChangeText={(t) => setNames(names.map((old, j) => (j === i ? t : old)))}
          placeholder={`Relay ${i + 1}`}
          placeholderTextColor={C.dim}
          style={styles.input}
        />
      ))}

      {error !== '' && <Text style={styles.panelError}>{error}</Text>}

      <Pressable
        style={({ pressed }) => [styles.saveBtn, (pressed || checking) && { opacity: 0.7 }]}
        onPress={connect}
        disabled={checking}
      >
        {checking ? <ActivityIndicator color={C.bg} /> : <Text style={styles.saveText}>CONNECT</Text>}
      </Pressable>
    </View>
  );
}

function RelayCard({ index, name, on, busy, disabled, onToggle }) {
  return (
    <Pressable
      onPress={() => !disabled && !busy && onToggle(!on)}
      style={[styles.card, on && styles.cardOn, disabled && { opacity: 0.5 }]}
    >
      <View style={styles.cardIcon}>
        <Text style={{ fontSize: 20 }}>{ICONS[index]}</Text>
      </View>
      <View style={styles.flex1}>
        <Text style={styles.cardName} numberOfLines={1}>{name}</Text>
        <Text style={styles.cardSub}>RELAY {index + 1}</Text>
      </View>
      {busy ? (
        <ActivityIndicator color={C.accent} style={{ marginRight: 8 }} />
      ) : (
        <Text style={[styles.stateText, { color: on ? C.on : C.off }]}>{on ? 'ON' : 'OFF'}</Text>
      )}
      <Switch
        value={on}
        onValueChange={onToggle}
        disabled={disabled || busy}
        trackColor={{ false: '#1a2535', true: 'rgba(0,255,136,0.35)' }}
        thumbColor={on ? C.on : C.dim}
      />
    </Pressable>
  );
}

function MasterButton({ label, color, disabled, onPress }) {
  return (
    <Pressable
      onPress={onPress}
      disabled={disabled}
      style={({ pressed }) => [
        styles.masterBtn,
        { borderColor: color, opacity: disabled ? 0.4 : pressed ? 0.7 : 1 },
      ]}
    >
      <Text style={[styles.masterText, { color }]}>{label.toUpperCase()}</Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  fill: { flex: 1, backgroundColor: C.bg },
  flex1: { flex: 1 },
  center: { alignItems: 'center', justifyContent: 'center' },
  content: { padding: 16, gap: 12 },
  header: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  logo: { color: C.accent, fontSize: 26, fontWeight: '900', letterSpacing: 3 },
  statusRow: { flexDirection: 'row', alignItems: 'center', gap: 8, marginTop: 6 },
  dot: { width: 8, height: 8, borderRadius: 4 },
  statusText: { color: C.dim, fontSize: 11, letterSpacing: 1.5, flexShrink: 1 },
  iconBtn: {
    width: 44,
    height: 44,
    borderRadius: 10,
    borderWidth: 1,
    borderColor: C.border,
    alignItems: 'center',
    justifyContent: 'center',
  },
  iconBtnText: { color: C.accent, fontSize: 22 },
  panel: { backgroundColor: C.panel, borderColor: C.border, borderWidth: 1, borderRadius: 12, padding: 16, gap: 8 },
  panelTitle: { color: C.dim, fontSize: 11, letterSpacing: 3, fontWeight: '700' },
  panelNote: { color: C.dim, fontSize: 12 },
  panelError: { color: C.off, fontSize: 13, lineHeight: 19 },
  input: {
    borderWidth: 1,
    borderColor: C.border,
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 10,
    color: C.text,
    fontSize: 16,
    backgroundColor: C.bg,
  },
  saveBtn: { marginTop: 8, backgroundColor: C.accent, borderRadius: 8, paddingVertical: 12, alignItems: 'center' },
  saveText: { color: C.bg, fontWeight: '800', letterSpacing: 3 },
  card: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
    backgroundColor: C.panel,
    borderColor: C.border,
    borderWidth: 1,
    borderRadius: 12,
    padding: 16,
  },
  cardOn: { borderColor: C.on },
  cardIcon: {
    width: 44,
    height: 44,
    borderRadius: 10,
    borderWidth: 1,
    borderColor: C.border,
    alignItems: 'center',
    justifyContent: 'center',
  },
  cardName: { color: C.text, fontSize: 17, fontWeight: '700' },
  cardSub: { color: C.dim, fontSize: 11, letterSpacing: 2, marginTop: 2 },
  stateText: { fontWeight: '800', letterSpacing: 2, marginRight: 4 },
  masterRow: { flexDirection: 'row', gap: 12, marginTop: 4 },
  masterBtn: { flex: 1, borderWidth: 1, borderRadius: 8, paddingVertical: 14, alignItems: 'center' },
  masterText: { fontWeight: '800', letterSpacing: 3 },
  hint: { color: C.off, fontSize: 13, lineHeight: 19, textAlign: 'center', marginTop: 4 },
});
