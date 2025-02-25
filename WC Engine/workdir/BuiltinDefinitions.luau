type Blaze = {
    write: <T...>(T...) -> (),
    info: <T...>(T...) -> (),
    warn: <T...>(T...) -> (),
    error: <T...>(T...) -> ()
}
declare blaze: Blaze