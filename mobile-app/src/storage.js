import AsyncStorage from '@react-native-async-storage/async-storage';

const KEY = 'relayapp:settings';

export const DEFAULT_SETTINGS = {
  host: '',
  names: ['Bedroom', 'Hall', 'Kitchen', 'Relay 4'],
};

export async function loadSettings() {
  try {
    const raw = await AsyncStorage.getItem(KEY);
    return raw ? { ...DEFAULT_SETTINGS, ...JSON.parse(raw) } : DEFAULT_SETTINGS;
  } catch {
    return DEFAULT_SETTINGS;
  }
}

export async function saveSettings(settings) {
  try {
    await AsyncStorage.setItem(KEY, JSON.stringify(settings));
  } catch {
    // Not fatal: settings just won't persist across restarts.
  }
}
