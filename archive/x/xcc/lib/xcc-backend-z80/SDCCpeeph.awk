BEGIN {
    print "/*"
    print " * Generated file, DO NOT Edit!"
    print " * To make changes to rules, edit <port>/peeph.def instead."
    print " */"
}

{
    printf "\""
    printf "%s", $0
    print "\\n\""
}
