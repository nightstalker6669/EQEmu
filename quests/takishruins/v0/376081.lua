local por = require("rof2_por_progression")

function event_say(e)
    if por.public(e.self, 376, 376081, 106901) and por.words(e) == "hail" then
        por.advance(e.other, 920307, 0)
    end
end
