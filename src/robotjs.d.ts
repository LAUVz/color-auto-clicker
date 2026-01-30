declare module 'robotjs' {
  export function getMousePos(): { x: number; y: number };
  export function getPixelColor(x: number, y: number): string;
  export function mouseClick(button?: 'left' | 'right' | 'middle', double?: boolean): void;
  export function moveMouse(x: number, y: number): void;
  export function keyTap(key: string, modifier?: string | string[]): void;
  export function typeString(string: string): void;
  export function getScreenSize(): { width: number; height: number };
}
