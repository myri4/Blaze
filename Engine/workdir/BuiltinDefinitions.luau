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


declare log: Log
declare FrozenTable: frozenTable
declare IsKeyPressed: (number) -> boolean
declare vec2: (x: number?, y: number?) -> vec2
declare vec3: (x: number?, y: number?, z: number?) -> vec3
declare vec4: (x: number?, y: number?, z: number?, w: number?) -> vec4