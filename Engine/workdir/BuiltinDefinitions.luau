type Log = {
    info: <T...>(T...) -> (),
    warn: <T...>(T...) -> (),
    error: <T...>(T...) -> (),
    critical: <T...>(T...) -> (),
}

type vec2 =
{
    x: number,
    y: number
}

type vec3 =
{
    x: number,
    y: number,
    z: number
} |
{
    r: number,
    g: number,
    b: number
}

type vec4 =
{
    x: number,
    y: number,
    z: number,
    w: number
} |
{
    r: number,
    g: number,
    b: number,
    a: number
}

type frozenTable = {
    Red: number,
    Green: number,
    Blue: number
}

type SoundID = {
    name: string,
    id: number
}


declare log: Log
declare FrozenTable: frozenTable
declare IsKeyPressed: (number) -> boolean

declare LoadSound: (string) -> number
declare UnloadSound: (number) -> ()
declare ReloadSound: (number, string) -> ()
declare InitializeSoundInstance: (number) -> SoundID
declare UninitializeSoundInstance: (SoundID) -> ()
declare UninitializeSound: (number) -> ()
declare ReloadSoundInstance: (SoundID) -> ()
declare PlaySound: (SoundID) -> ()
declare PauseSound: (SoundID) -> ()

declare vec2: (x: number?, y: number?) -> vec2
declare vec3: (x: number?, y: number?, z: number?) -> vec3
declare vec4: (x: number?, y: number?, z: number?, w: number?) -> vec4

declare SoundID: (name: string, id: number) -> SoundID

declare normalize: (v: vec2) -> vec2
declare normalize: (v: vec3) -> vec3
declare normalize: (v: vec4) -> vec4

declare min: (v1: vec2, v2: vec2) -> vec2
declare min: (v1: vec3, v2: vec3) -> vec3
declare min: (v1: vec4, v2: vec4) -> vec4

declare max: (v1: vec2, v2: vec2) -> vec2
declare max: (v1: vec3, v2: vec3) -> vec3
declare max: (v1: vec4, v2: vec4) -> vec4

declare length: (v: vec2) -> number
declare length: (v: vec3) -> number
declare length: (v: vec4) -> number

declare distance: (v1: vec2, v2: vec2) -> number
declare distance: (v1: vec3, v2: vec3) -> number
declare distance: (v1: vec4, v2: vec4) -> number

declare cross: (v1: vec3, v2: vec3) -> vec3

declare dot: (v1: vec2, v2: vec2) -> number
declare dot: (v1: vec3, v2: vec3) -> number
declare dot: (v1: vec4, v2: vec4) -> number