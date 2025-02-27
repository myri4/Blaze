type Blaze = {
    info: <T...>(T...) -> (),
    warn: <T...>(T...) -> (),
    error: <T...>(T...) -> (),
    critical: <T...>(T...) -> ()
}
declare blaze: Blaze